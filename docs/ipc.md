# Inter-Process Communication

## Header
```
 63          48 47         32 31   26 25   16 15          0
+--------------+-------------+-------+-------+-------------+
|  channel ID  |   accept    | flags | types |   label     |
+--------------+-------------+-------+-------+-------------+
```

### Flags
- `bit 0`: send
- `bit 1`: recv
- `bit 2`: reply
- `bit 3`: unblocking send
- `bit 4`: unblocking recv
- `bit 5`: reserved

### Types
- `0xb00`: inline payload
- `0xb01`: page payload

## Payloads
### Page
```
 63                                12 11   8 7     3 2    0
+------------------------------------+------+-------+------+
|               offset >> 12         | rsvd |  exp  | type |
+------------------------------------+------+-------+------+
```

#### Types
- `0xb00`: move
- `0xb01`: shared
