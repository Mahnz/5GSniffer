from __future__ import annotations
import time
import threading
from typing import Dict, Tuple, List, Optional
from .schemas import RntiSnapshot, RntiDelta


class RntiState:
    """
    Thread-safe in-memory state table with TTL-based expiration.
    Maintains active RNTIs and a small ring buffer of recently expired.
    """

    def __init__(self, ttl_seconds: float = 30.0, expired_buffer_size: int = 1024):
        self._ttl = float(ttl_seconds)
        self._active: Dict[int, RntiSnapshot] = {}
        self._recently_expired: List[dict] = []
        self._total_seen = 0
        self._mu = threading.RLock()

    # ---- mutate ----
    def upsert(self, delta: RntiDelta) -> Tuple[str, RntiSnapshot]:
        """Insert/update a snapshot. Returns (event, snapshot) where event is "new" or "update"."""
        snap = delta.snapshot
        now = snap.t_seconds
        with self._mu:
            existed = snap.rnti in self._active
            if existed:
                prev = self._active[snap.rnti]
                # Preserve first_seen
                snap.first_seen = prev.first_seen
            else:
                self._total_seen += 1
                if getattr(snap, "first_seen", None) is None:
                    snap.first_seen = now
            # Always refresh last_seen
            snap.last_seen = now
            # Normalize status to active
            snap.status = "active"
            self._active[snap.rnti] = snap
            return ("update" if existed else "new", snap)

    def mark_expired(self, rnti: int, when: float) -> Optional[RntiSnapshot]:
        with self._mu:
            snap = self._active.pop(rnti, None)
            if snap is None:
                return None
            self._recently_expired.append(
                {
                    "rnti": rnti,
                    "expired_at": when,
                    "last_seen": snap.last_seen,
                    "cell_id": snap.cell_id,
                }
            )
            # Bound buffer
            if len(self._recently_expired) > 1024:
                self._recently_expired = self._recently_expired[-1024:]
            return snap

    # ---- queries ----
    def snapshot(self) -> Tuple[float, List[RntiSnapshot], List[dict], dict]:
        with self._mu:
            now = time.time()
            active_list = list(self._active.values())
            rec_exp = list(self._recently_expired[-128:])  # only show some
            stats = {"active_count": len(self._active), "total_seen": self._total_seen}
            return now, active_list, rec_exp, stats

    def expire_due(self) -> List[int]:
        """Return RNTIs that have exceeded TTL."""
        now = time.time()
        due = []
        with self._mu:
            for rnti, snap in self._active.items():
                if now - snap.last_seen >= self._ttl:
                    due.append(rnti)
        return due

    @property
    def ttl(self) -> float:
        return self._ttl
