/* Create a test which unblocks a task from an external thread. This
 * should break currently, as the instrumentation attempts to call
 * ovni_ev_emit from a thread without initialization. */
