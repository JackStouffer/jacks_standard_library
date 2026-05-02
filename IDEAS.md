# Ideas

Nice to haves or ideas for enhancement

* Need a tool for serialization
* need fixed and dynamic arrays

## Subprocess Feedback

4. No graceful termination. background_kill only does SIGKILL/TerminateProcess. There's no SIGTERM / CTRL_BREAK_EVENT, and no way to send arbitrary signals (SIGUSR1, SIGHUP). The typical pattern is
"SIGTERM, wait N seconds, SIGKILL". Add jsl_subprocess_background_signal(proc, sig) on POSIX and a terminate on Windows.
6. Environment semantics are surprising.
- The doc says repeated jsl_subprocess_env(key, ...) "appends a duplicate entry; no deduplication." That's a footgun — depending on platform/libc the child may see either value, and users will hit it. At minimum dedupe on key; better, document and enforce override-last-wins.
- There's no way to (a) start from a clean environment, (b) remove a single var (unsetenv semantics), or (c) ask "inherit parent + override these." Right now if you call _env once, is it additive over the   parent's env or replaces it? The header doesn't say. Worth a _env_clear and a _env_inherit_parent(bool) toggle, or document the default explicitly.
7. PATH-search behavior is undocumented and platform-divergent. POSIX uses posix_spawnp (searches PATH); Windows uses CreateProcessA (which has its own resolution rules and won't search PATH for paths containing slashes the same way). Users will hit "works on Linux, fails on Windows." Either pick one behavior and document it, or add an explicit search_path flag.
8. No shell invocation helper. People will reach for sh -c "foo | bar" constantly. Either add jsl_subprocess_create_shell or document the recipe in the header.
9. No process group / session control. No start_new_session / setpgid (POSIX) or CREATE_NEW_PROCESS_GROUP / DETACHED_PROCESS (Windows). Important for daemonizing, for kill-the-whole-tree, and for preventing   Ctrl+C from propagating into the child.
10. No fd-inheritance control. When the user passes set_stdin_fd(fd), what happens to other open fds in the parent? POSIX posix_spawn inherits everything not marked O_CLOEXEC; Windows inheritance is
per-handle. Inherited-by-accident fds are a classic source of "child holds a port the parent tried to close" bugs. The header should at least state the policy.
12. Setters return bool, configuration calls return enums. set_stdin_memory, set_stdout_sink, change_working_directory all return bool, while subprocess_create, _args, _env return enums distinguishing
BAD_PARAMETERS vs COULD_NOT_ALLOCATE. Inconsistent — and the setters can fail for either reason, so they're losing information. Pick one.
13. TODO: docs markers on JSLSubProcessCreateResultEnum, JSLSubProcessArgResultEnum, JSLSubProcessEnvResultEnum, and JSLSubProcessResultEnum (lines 673, 685, 697, 1126). The result enums are part of the public API and should be documented.
15. Windows takes CRT fds, not HANDLEs. Native Windows code overwhelmingly speaks HANDLE. Forcing a _open_osfhandle round-trip is awkward. Consider an additional set_stdin_handle / set_stdout_handle on Windows.
16. exit_code overloads "exit code OR negated signal" based on status. Workable but easy to misuse if someone reads exit_code without checking status. A separate signal_number field, or a tagged accessor,
would prevent that. Python keeps returncode but signals show up as negative — same convention you've picked, so this is at least familiar.
17. No way to retrieve raw pid/HANDLE through a public accessor. Users sometimes need it for OS-specific operations (waitid, OpenProcessToken, attaching a debugger, sending custom signals). Today you'd have   to reach into struct internals.

Smaller things

- jsl_subprocess_create reasonably treats executable as both program path and argv[0]. Go separates Cmd.Path and Cmd.Args[0]; rare to need but worth a one-line note that they're conflated.
- The argv variadic macro relies on a (NULL, -1) sentinel — fine, but a 0-arg call (jsl_subprocess_arg(&p)) will produce a confusing error. Consider a _clear_args for that intent.
- set_stdin_fd doesn't say when the fd is consumed and when the caller may close it. Spell out: "fd must remain open until run_blocking/background_start returns."
- Background-mode *_eof_seen and stdin_write_offset are visible struct fields; if they're not part of the API, hide them behind jsl__ or note they're internal.

# Subprocess 

Smaller notes on what's already there. Documenting the Windows arg-quoting rules the implementation follows (MSDN CommandLineToArgvW conventions) is worth it — callers get bitten by this constantly. And for the streaming-stdin case (feed bytes to a live child over time), MEMORY isn't enough; you'd want either a stdin FD the caller writes to, or a stdin-sink-as-callback. Probably a v2 concern.
