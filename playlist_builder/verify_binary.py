"""Validate the Rockbox 4.0 iPod Video header and address bounds."""
import hashlib
import struct
import sys
from pathlib import Path

path = Path(sys.argv[1])
data = path.read_bytes()
magic, target, api, start, end, entry, rb_ptr, api_size = struct.unpack('<IHHIIIII', data[:28])
assert (magic, target, api) == (0x526F634B, 15, 273)
assert start == 0x03F80000
assert start + len(data) <= end <= start + 0x80000
assert start <= entry < start + len(data)
assert start <= rb_ptr < end
assert api_size == 1644, 'Unexpected API layout; review target configuration'
print(f'PASS: target={target}, API={api}, file={len(data)} bytes, RAM={end-start} bytes, API table={api_size} bytes')
print('SHA256:', hashlib.sha256(data).hexdigest())
