# ft_irc — IRC Server in C++98

> A fully RFC-compliant Internet Relay Chat server built from scratch using non-blocking sockets and `poll()` multiplexing. Supports concurrent clients, channel management, operator privileges, and works out of the box with standard IRC clients like WeeChat and irssi.

*Built as part of the 42 curriculum by **msylaiev**, **dkot**, **elsurovt**.*

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Features](#features)
- [Supported Commands](#supported-commands)
- [Channel Modes](#channel-modes)
- [Build & Run](#build--run)
- [Connection Flow](#connection-flow)
- [Protocol Compliance](#protocol-compliance)
- [Project Structure](#project-structure)
- [Testing](#testing)
- [References](#references)

---

## Overview

`ft_irc` is a single-threaded, event-driven IRC server that handles multiple concurrent clients without threads or forking. It uses the POSIX `poll()` system call to multiplex I/O across all file descriptors — the same approach used by production IRC daemons and network servers.

**Why this approach?**

| Method | Concurrency | Complexity | Scalability |
|---|---|---|---|
| One thread per client | Linear | Low | Poor (stack overhead) |
| `select()` | Event-driven | Low | Limited (FD_SETSIZE) |
| `poll()` | Event-driven | Medium | Good (dynamic array) |
| `epoll()` | Event-driven | High | Excellent |

`poll()` sits at the sweet spot for this project: no artificial FD limits, straightforward API, and available on every POSIX system. The server handles up to **128 simultaneous clients** on a single thread with no busy-waiting.

---

## Architecture

```
                        ┌──────────────────────────────────────────┐
                        │                server.cpp                │
                        │                                          │
                        │   socket() → bind() → listen()           │
                        │              │                            │
                        │         poll() loop ◄──────────────────┐ │
                        │              │                          │ │
                        │    ┌─────────┴──────────┐              │ │
                        │    │                    │              │ │
                        │  new conn           read data          │ │
                        │  accept()        → recv() into buf     │ │
                        │    │                    │              │ │
                        │    │            CommandProcessor       │ │
                        │    │           (split on \r\n)         │ │
                        │    │                    │              │ │
                        │    │              Parser::parse()      │ │
                        │    │           (prefix/cmd/params)     │ │
                        │    │                    │              │ │
                        │    │            Dispatcher::dispatch() │ │
                        │    │           (command → handler)     │ │
                        │    │                    │              │ │
                        │    │            Handlers:: *           │ │
                        │    │           (mutate ServerState)    │ │
                        │    │                    │              │ │
                        │    └──── flush outbox ──┘              │ │
                        │              send() all queued msgs ───┘ │
                        └──────────────────────────────────────────┘

      ServerState
      ┌───────────────────────────────┐
      │  map<int, Client>             │  fd → client data
      │  map<string, int> nickToFd    │  nickname → fd
      │  map<string, Channel> channels│  name → channel data
      │  string password              │
      └───────────────────────────────┘
```

**Key design decisions:**

- **No threads.** All I/O is non-blocking; `poll()` handles all event demultiplexing.
- **Per-client buffering.** Each client has an input buffer (accumulated raw bytes) and an outbox (messages to send next iteration). This decouples reading from writing and prevents partial-write issues.
- **Layered processing.** Raw bytes → complete lines → parsed `Message` → dispatched handler → mutated state → queued replies. Each layer has a single responsibility.
- **Automatic channel cleanup.** Channels are destroyed when the last member leaves. No stale state.
- **Operator auto-assignment.** The first user to join a channel becomes its operator automatically.

---

## Features

**Network layer**
- Non-blocking TCP server socket with `SO_REUSEADDR`
- `poll()`-based event loop — no threads, no forking
- Incremental read buffering with `\r\n` line detection
- Per-client message outbox flushed every iteration

**Registration & authentication**
- Server password enforcement via `PASS` before registration
- Nickname validation (RFC-compliant character set and length)
- Full `USER` command handling (username, mode, realname)
- Capability negotiation (`CAP LS/END`) for WeeChat/irssi compatibility
- Welcome burst (RPL 001–004) on successful registration

**Channel management**
- Unlimited channels, unlimited members per channel (up to server cap)
- Invite-only (`+i`), key-protected (`+k`), user-limited (`+l`), topic-locked (`+t`) modes
- Operator management (`+o`/`-o`) with per-command privilege checks
- `INVITE` list: invited users bypass `+i` restriction
- Channel auto-creation on first `JOIN`, auto-deletion on last `PART`/`QUIT`

**Messaging**
- `PRIVMSG` to channel (broadcast to all members except sender)
- `PRIVMSG` to nickname (direct private message)
- `KICK` with optional reason, broadcast to channel
- `TOPIC` view and set (respects `+t` operator restriction)

**Protocol compliance**
- RFC 1459 / RFC 2812 numeric reply codes throughout
- Correct IRC message format: `:prefix COMMAND params :trailing\r\n`
- `PING`/`PONG` keep-alive handling

---

## Supported Commands

| Command | Description | Requires Registration |
|---|---|---|
| `CAP` | Capability negotiation (LS, END) | No |
| `PASS` | Authenticate with server password | No |
| `NICK` | Set or change nickname | No |
| `USER` | Set username and realname | No |
| `PING` | Keep-alive, server responds with PONG | Yes |
| `JOIN` | Join one or more channels (with optional keys) | Yes |
| `PART` | Leave a channel | Yes |
| `PRIVMSG` | Send message to channel or user | Yes |
| `TOPIC` | View or set channel topic | Yes |
| `MODE` | Get or set channel modes | Yes |
| `KICK` | Remove a user from a channel (operator only) | Yes |
| `INVITE` | Invite a user to a channel (operator only) | Yes |
| `QUIT` | Disconnect from server | Yes |

---

## Channel Modes

| Mode | Flag | Argument | Description |
|---|---|---|---|
| Invite-only | `+i` / `-i` | — | Only invited users can join |
| Topic lock | `+t` / `-t` | — | Only operators can change topic |
| Channel key | `+k` / `-k` | `<key>` | Require password to join |
| User limit | `+l` / `-l` | `<count>` | Maximum number of members |
| Operator | `+o` / `-o` | `<nick>` | Grant or revoke operator status |

**Example:**
```
MODE #general +itk secretpass     → invite-only, topic-locked, password-protected
MODE #general +l 10               → limit to 10 users
MODE #general +o alice            → make alice an operator
MODE #general -i                  → remove invite-only restriction
```

---

## Build & Run

### Requirements

- Linux or any POSIX-compatible system
- C++ compiler with C++98 support (`g++` or `c++`)
- GNU `make`

### Build

```bash
git clone <repo-url>
cd ircr
make
```

This produces the `ircserv` binary. Object files are placed in `obj/`.

```bash
make clean    # remove object files
make fclean   # remove objects and binary
make re       # full rebuild
```

### Run

```bash
./ircserv <port> <password>
```

**Example:**
```bash
./ircserv 6667 hunter2
```

The server starts listening on `0.0.0.0:<port>`. Connect any IRC client to `localhost:<port>` with the given password.

### Connect with WeeChat

```
/server add local localhost/6667 -password=hunter2 -nicks=mynick
/connect local
```

### Connect with irssi

```
/connect -password hunter2 localhost 6667
```

### Connect with netcat (manual testing)

```bash
nc localhost 6667
PASS hunter2
NICK testuser
USER testuser 0 * :Test User
JOIN #general
PRIVMSG #general :Hello, world!
```

---

## Connection Flow

```
Client                          Server
  │                               │
  │──── TCP connect ─────────────►│  accept(), add to poll set
  │                               │
  │──── CAP LS ──────────────────►│  :server CAP * LS :
  │──── CAP END ─────────────────►│  (acknowledged)
  │──── PASS hunter2 ────────────►│  passAccepted = true
  │──── NICK alice ──────────────►│  hasNick = true
  │──── USER alice 0 * :Alice ───►│  hasUser = true
  │                               │  → registered = true
  │◄─── :server 001 alice :Welcome│  RPL_WELCOME
  │◄─── :server 002 alice :Host   │  RPL_YOURHOST
  │◄─── :server 003 alice :Created│  RPL_CREATED
  │◄─── :server 004 alice :...    │  RPL_MYINFO
  │                               │
  │──── JOIN #general ───────────►│  channel created, alice → operator
  │◄─── :alice JOIN #general      │  broadcast to channel
  │◄─── :server 353 alice = #g :@alice│  RPL_NAMREPLY
  │◄─── :server 366 alice #g :End │  RPL_ENDOFNAMES
  │                               │
  │──── PRIVMSG #general :Hi ────►│  broadcast to channel members
  │──── QUIT :bye ───────────────►│  broadcast QUIT, remove client
  │                               │  close fd
```

---

## Protocol Compliance

The server implements the following numeric replies from RFC 1459 and RFC 2812:

| Code | Name | Trigger |
|---|---|---|
| 001 | RPL_WELCOME | Successful registration |
| 002 | RPL_YOURHOST | Successful registration |
| 003 | RPL_CREATED | Successful registration |
| 004 | RPL_MYINFO | Successful registration |
| 353 | RPL_NAMREPLY | After JOIN, NAMES |
| 366 | RPL_ENDOFNAMES | After RPL_NAMREPLY |
| 331 | RPL_NOTOPIC | TOPIC query, no topic set |
| 332 | RPL_TOPIC | TOPIC query, topic set |
| 341 | RPL_INVITING | Successful INVITE |
| 421 | ERR_UNKNOWNCOMMAND | Unrecognized command |
| 431 | ERR_NONICKNAMEGIVEN | NICK with no argument |
| 432 | ERR_ERRONEUSNICKNAME | Invalid nickname characters |
| 433 | ERR_NICKNAMEINUSE | Nickname already taken |
| 436 | ERR_NICKCOLLISION | Nick collision |
| 451 | ERR_NOTREGISTERED | Command before registration |
| 461 | ERR_NEEDMOREPARAMS | Missing required parameters |
| 462 | ERR_ALREADYREGISTERED | Repeated PASS/USER |
| 471 | ERR_CHANNELISFULL | Channel at user limit |
| 473 | ERR_INVITEONLYCHAN | Not invited to +i channel |
| 474 | ERR_BANNEDFROMCHAN | Banned (not implemented, reserved) |
| 475 | ERR_BADCHANNELKEY | Wrong key for +k channel |
| 482 | ERR_CHANOPRIVSNEEDED | Operation requires operator |

---

## Project Structure

```
ircr/
├── server.cpp                   # Entry point, socket setup, poll() event loop
├── Makefile
├── parser/
│   ├── Parser.hpp / Parser.cpp  # IRC message parser (prefix, command, params)
│   └── Message.hpp / Message.cpp# Message struct
└── handlers/
    ├── ServerState.hpp / .cpp   # Global server state (clients, channels, password)
    ├── State.hpp / .cpp         # State accessor and mutator operations
    ├── CommandProcessor.hpp/.cpp# Raw buffer → complete lines pipeline
    ├── Dispatcher.hpp / .cpp    # Command name → handler routing
    ├── Handlers.hpp / .cpp      # All IRC command implementations
    └── SendHelpers.hpp / .cpp   # Message formatting and send utilities
```

**Component responsibilities:**

| Component | Responsibility |
|---|---|
| `server.cpp` | Socket lifecycle, `poll()` loop, connection accept, buffer flush |
| `Parser` | Tokenize raw IRC line into `Message` (prefix, command, params) |
| `CommandProcessor` | Split recv buffer on `\r\n`, call Parser per line |
| `Dispatcher` | Map command string to handler function |
| `Handlers` | Implement each IRC command, validate args, mutate state |
| `State` | Safe accessors for clients/channels; registration logic |
| `ServerState` | POD container for all mutable server data |
| `SendHelpers` | Format and enqueue reply messages |

---

## Testing

### Basic smoke test with `nc`

```bash
# Terminal 1 — start server
./ircserv 6667 pass

# Terminal 2 — client A
nc localhost 6667
PASS pass
NICK alice
USER alice 0 * :Alice
JOIN #test
PRIVMSG #test :hello from alice

# Terminal 3 — client B
nc localhost 6667
PASS pass
NICK bob
USER bob 0 * :Bob
JOIN #test
PRIVMSG #test :hi alice!
```

### Operator flow

```
# As alice (auto-op after creating channel):
MODE #test +i               # make invite-only
INVITE bob #test            # invite bob explicitly
KICK bob #test :bye bob     # remove bob
MODE #test +o bob           # re-grant op to bob
MODE #test +kl secret 5     # password + limit
```

### Edge cases verified

- Reconnect after QUIT reuses nickname correctly
- Partial `\r\n` in recv buffer handled without data loss
- Channel cleanup when last member leaves
- Duplicate NICK collision rejected with ERR_NICKNAMEINUSE
- MODE with no args returns current mode string
- WeeChat CAP negotiation completes without errors

---

## References

- [RFC 1459 — Internet Relay Chat Protocol](https://www.rfc-editor.org/rfc/rfc1459)
- [RFC 2812 — IRC: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [`poll(2)` man page](https://man7.org/linux/man-pages/man2/poll.2.html)
- [Modern IRC numerics reference](https://modern.ircdocs.horse/)
- [C++ reference (C++98)](https://en.cppreference.com/)


---

## License

MIT © [denyskotcode](https://github.com/denyskotcode) — see [LICENSE](LICENSE) for details.