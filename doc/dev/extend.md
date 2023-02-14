# Extend

The extend mechanism allows a structure to be extended by a model. It works by
placing an array of pointers indexed by the model identifier (char) inside the
structure. Most of the structures of the emulator have a `ext` member, allowing
them to be extended.

Models are forbidden to access information from other models than their own.

The function `void extend_set(struct extend *ext, int id, void *ctx)` stores the
ctx pointer inside the extend structure for the model with the given id.

Use `void *extend_get(struct extend *ext, int id)` to retrieve its value.

A helper macro `EXT(st, m)` directly attempts to find the member `ext` in the
structure st, and return the pointer of the model `m`.

Here is an example where Nanos6 stores its own CPU information:

    struct cpu *syscpu;
    struct nanos6_cpu *cpu;

	extend_set(&syscpu->ext, '6', cpu);

	cpu = extend_get(&syscpu->ext, '6');
	cpu = EXT(syscpu, '6'); /* same */
