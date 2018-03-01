# Introduction

## Design Choices

### Message Format

Messages are exchanged in a three step format. First the 'length' of the incoming message is sent (in 4 bytes), then the 'type' of the message is sent (in 4 bytes) and finally the actual message is sent (in length bytes). This three step format applies to all message types except the termination message. The termination is sent in a two step format. First the 'length' of the incoming message is sent (in 4 bytes) and then the 'type' of the message is sent (in 4 bytes).

### Specific to librpc.a

### Specific binder.c


## Error Codes


## Functions Not Implemented


## Advanced Enhancement
