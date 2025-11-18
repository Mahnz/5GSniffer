from pydantic import BaseModel, Field
from typing import Optional, Literal


class RntiSnapshot(BaseModel):
    rnti: int
    cell_id: Optional[int] = None
    scrambling_id: Optional[int] = None
    coreset_id: Optional[int] = None

    t_seconds: float = Field(..., description="Timestamp from sniffer")
    sample_index: Optional[int] = None

    seen_count: int = 0
    revivals: int = 0
    status: Literal["active", "inactive"] = "active"

    last_seen: float
    first_seen: float


class RntiDelta(BaseModel):
    event: Literal["new", "update", "expire"]
    snapshot: RntiSnapshot


class SnapshotResponse(BaseModel):
    now: float
    stats: dict
    active: list[RntiSnapshot]
    recently_expired: list[dict]
