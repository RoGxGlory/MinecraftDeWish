# Networking / Replication Rules

Applies to replicated actors, components, and RPCs.

- The server is authoritative. Clients request actions; the server validates and replicates results.
- Replicate only what is necessary. Prefer `ReplicatedUsing` with a callback over polling in `Tick`.
- Mark replicated properties with `UPROPERTY(Replicated)` or `UPROPERTY(ReplicatedUsing=OnRep_X)` and implement `GetLifetimeReplicatedProps`.
- Use RPCs sparingly: `Server` for client-to-server requests, `Client` for targeted feedback, `NetMulticast` for cosmetic broadcasts.
- Never let a client decide damage, loot, inventory contents, or world state.
- Validate every client RPC (range, ownership, cooldown, state) on the server.
- Use `FNetBitArray` / quantized types / `FVector_NetQuantize` for bandwidth-sensitive data where appropriate.
- Cosmetic effects (sounds, particles, decals) should be multicast or client-local, not authoritative state.
- Test with simulated latency (`Net PktLag=100`) and packet loss before calling a multiplayer feature done.
- Replication changes require an ADR if they alter ownership, authority, or save data.
