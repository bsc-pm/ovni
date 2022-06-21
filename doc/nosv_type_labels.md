# nOS-V task type colors

The color assigned to each nOS-V task type is computed from the task
type label using a hash function; the task type id doesn't affect in any
way how the color gets assigned. This method provides two desirable
properties:

- Invariant type colors over time: the order in which task types are
  created doesn't affect their color.

- Deterministic colors among threads: task types with the same label end
  up mapped to the same color, even if they are from different threads
  located in different nodes.

For more details, see [this MR][1].

[1]: https://pm.bsc.es/gitlab/rarias/ovni/-/merge_requests/27
