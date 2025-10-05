#!/usr/bin/env bash

VENDOR_FILTER=2cf0: # Nuand
DOCKER_IMAGE_NAME=5gsniffer
CONTAINER_NAME=5gsniffer-cnt
DOCKER_IMAGE_TAG=latest

mapfile -t devices < <(lsusb -d ${VENDOR_FILTER} | tr ':' ' ')

if [[ "${#devices[@]}" -eq "0" ]]; then
  echo "No BladeRFs found!"
  exit 1
elif [[ "${#devices[@]}" -ne "1" ]]; then
  for i in "${!devices[@]}"; do
    printf "%s) %s\n" "$i" "${devices[$i]}"
  done

  IFS= read -r -p "Select a USB device to mount for this container: " seldev
  if ! [[ $seldev =~ ^[0-9]+$ ]] || ! (((seldev >= 0) && (seldev <= "${#devices[@]}"))); then
    echo Abort
    exit 1
  fi
else
  seldev=0
fi

DEVICE_FILE=$(printf "%s" "${devices[$seldev]}" | awk '{ print "/dev/bus/usb/"$2"/"$4; }')
echo 'performance' |
  sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor >/dev/null

echo "Using $DEVICE_FILE"

docker run \
  --rm \
  --interactive \
  --tty \
  --volume "$PWD:/workspace" \
  --name "$CONTAINER_NAME" \
  --device "$DEVICE_FILE" \
  "$DOCKER_IMAGE_NAME:$DOCKER_IMAGE_TAG" bash
