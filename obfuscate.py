#!/usr/bin/env python3

import secrets

# Query API key from user
apikey = input("Please enter the API key: ").strip()
# Reverse key, and convert it to a list of ASCII int
apikey = [ord(c) for c in reversed(apikey)]
# Check that API key only contains valid, printable ASCII int
assert all(32 <= c < 128 for c in apikey)

# Generate a randomize int list matching the API key in length
xorbytes = [int(a) for a in secrets.token_bytes(len(apikey))]
# Avoid situations were XOR operation (see below) results in \x00 or the XOR byte is 0
for i, c in enumerate(apikey):
    while c == xorbytes[i] or xorbytes[i] == 0:
        xorbytes[i] = int(secrets.token_bytes(1)[0])

# Create list of two-entry lists, where the first of the two entries is a byte from the
# API key XOR-ed with a random byte and the other entry is the same random byte
obfuscated = [[xorbytes[i] ^ c, xorbytes[i]] for i, c in enumerate(apikey)]
# Resolve the list of two-entry lists into a planar list
obfuscated = [b for o in obfuscated for b in o]
# Convert the list of numbers into a string that encodes each number in a 2-digit hex number
# using C-style syntax to express a byte value
obfuscated = "".join(map(lambda b: f"\\x{b:02x}", obfuscated))[: len(apikey) * 8]

# Validate if obfuscated data looks ok
assert len(obfuscated) == len(apikey) * 8
assert not "\\x00" in obfuscated

print(obfuscated)
