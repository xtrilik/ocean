# Core: CLI Engine System PRD (v3)

## 1. Purpose

The Command-Line Interface (CLI) Engine System is a fundamental sub-component of the Launcher Core. Its primary purpose is to provide a robust, interactive, and scriptable text-based interface for users to manage, monitor, and diagnose the Launcher application and its modules. It handles command input, parsing, dispatching to registered command handlers, and formatting output.

## 2. Key Responsibilities

* Interactive prompt for user input (CLI) and a programmatic API for GUI or remote frontends.
* Reading and parsing command lines (including from startup or configuration scripts).
* Maintaining a thread-safe registry of available commands: names, aliases, help text, and handlers.
* Dispatching commands (sync or async) and returning structured results.
* Allowing external modules to register/unregister commands at runtime, including module-specific operational and diagnostic commands (e.g., `<module_name> set debug <on|off>`).
* Dynamic help system (`help <command>`, `<domain> show commands`) sourced from registered commands.
* Command history navigation (↑/↓) with optional persistent history file.
* **Tab-Completion:**

  * Complete first word from registered commands/aliases.
  * Designed to support future argument completion via per-command callbacks.
* Formatting clear, consistent output (plain text for CLI; structured for other consumers).
* Emitting events on registry changes and command execution for GUI/automation.

### 2.1 Naming Conventions & Aliases

Commands follow this structure:

```
<domain> <verb> <resource> [<identifier>] [<options>]
```

* **Domain**: feature area (e.g. `calendar`, `database`, `logger`, `clipboard`, `eventbus`, `opportunity`).
* **Verb**: imperative action (`show`, `set`, `reload`, `clear`, `exit`, `alias`, etc.).
* **Resource**: target of the verb (`events`, `channels`, `users`, `commands`, etc.).
* **Identifier**: optional key or name (`<family> <key>`, `<oppname>`, `<path>`).
* **Options**: flags or parameters (`--async`, `penalty <n>`).

> **Argument Quoting**
> If an argument contains spaces or special characters, wrap it in double quotes:
>
> ```
> clipboard save "/tmp/my file.txt"  
> ```

**Alias Support** (no permissions subsystem):

```
alias show
alias add <existing-command> <alias>
alias remove <alias>
alias reload
```

## 3. Key Internal Interfaces/Classes (Conceptual)

* **CLIEngine**

  * Orchestrates input loop, parsing, dispatch, history, and tab-completion.
  * Exposes methods for command processing: one suitable for direct string output (e.g., for a simple CLI), and another that returns a structured result (e.g., for GUI or programmatic use). Both approaches should accommodate synchronous and asynchronous execution and be designed with thread-safety considerations.

* **CommandRegistry**

  * A thread-safe store of all command implementations, typically keyed by a multi-part identifier (e.g., domain/verb/resource).
  * Provides capabilities to register, unregister, and find commands.

* **ICommand (or Command Definition)**

  * An interface or contract that command implementations must adhere to. It should provide:
    *   Mechanisms to retrieve the command's name and any aliases.
    *   A way to get a concise summary or one-line help text.
    *   A method to obtain detailed usage syntax and description.
    *   An execution mechanism that accepts a command context and arguments, returning a structured command result.
    *   Optionally, a way to provide argument completion suggestions based on a given prefix.

* **CommandContext**

  * Carries core access, output hooks, session state.

* **HistoryManager**

  * Stores per-session (and optionally persistent) command history.

* **InputParser**

  * Splits lines into tokens, honors quoting rules.

* **OutputFormatter**

  * Standardizes how results, tables, and errors are rendered.

### 3.1 Module Command Registration

Each module must register its CLI commands during initialization and unregister them on shutdown:

1. **On load/initialize():**
   Modules use the CommandRegistry to register their commands, providing necessary descriptor information (e.g. name, handler, help text).
   * Adds the module's commands (e.g. `calendar show events`, `database put …`).

2. **On shutdown/unload():**
   Modules use the CommandRegistry to unregister their commands, typically identified by their registration path or descriptor.
   * Removes the module's commands from the registry.

This dynamic registration makes commands appear or disappear as modules are loaded or unloaded.

### 3.2 Structured Results & Error Reporting

Commands should return a structured result containing:
  * A status indicator (e.g., Success, Warning, Error).
  * A message string describing the outcome.
  * Optional structured data for more complex results.

* All command handlers should return such a structured result for easy CLI/GUI consumption.

### 3.3 Threading & Async

* All public APIs thread-safe or clearly main-thread-only.
* Long-running commands support async (futures/promises or callbacks).

### 3.4 Extensibility Events

* Publish events on:

  * Command registration/unregistration
  * Command execution (start/finish)
* GUI/frontends subscribe to update menus, logs, etc.

## 4. Interactions with Other Core Sub-components

* **Module Loader:** modules utilize the CommandRegistry's capabilities to register/unregister commands on load/unload.
* **Event Bus:** CLI emits `cli.command.executed`, `cli.registry.changed`.
* **Logging System:** logs parsing errors, execution results.
* **Configuration System:** reads `[CLI]` settings (history, prompt, listen URI, output color).

## 5. Configuration

Read from `[CLI]` section of `launcher.conf`:

```
enable_history      = true
max_history_items   = 500
history_file        = "/home/user/.launcher_history"
prompt_string       = "launcher> "
listen_uri          = "tcp://127.0.0.1:12345"
output_use_color    = auto | yes | no
```

## 6. Output Formatting Standards

* Plain-text for CLI; for programmatic consumption (e.g., by a GUI or scripts), commands should provide results in a structured format (see Structured Results & Error Reporting).
* Consistent table and list layouts.

## 7. Error Handling

* All errors as `CommandResult{Status::Error, ...}`.
* Detailed diagnostics in logs.

## 8. Shell Meta-Commands

```
clear         # or cls — clear the console screen
exit, quit    # terminate the CLI session
```

## 9. Alias Management

```
alias show
alias add <command> <alias>
alias remove <alias>
alias reload
```

## 10. Logging Verbosity Control

```
cli set verbose <level>
```

* **Note:** Map `<level>` to your logging system's thresholds (e.g. 0=errors, 1=warnings, 2=info, 3=debug).

## 11. Remote Attach / Daemon Mode

```
cli listen unix:/path/to/socket
cli listen tcp://0.0.0.0:12345
cli stop-listen
```

* Supports headless mode; clients attach via `launcher-cli -r`.

## 12. Pagination (Long-Output Handling)

* Honor `$PAGER` if set (pipe through `less`).
* Or use a built-in `--pager` flag on long-output commands:

  ```
  calendar show events --pager
  ```

## 13. Command Index

### Core-Provided Commands

| Command                       | Description                                                              |
| ----------------------------- | ------------------------------------------------------------------------ |
| `core show help`, `help`      | Display help information for commands.                                   |
| `show commands`               | List every registered command across all domains.                        |
| `show commands all`           | Alias for `show commands`.                                               |
| `clear`, `cls`                | Clear the console screen.                                                |
| `exit`, `quit`                | Terminate the CLI session cleanly.                                       |
| `alias show`                  | Display all currently defined CLI aliases.                               |
| `alias add <command> <alias>` | Create a new alias mapping `<alias>` to the existing `<command>`.        |
| `alias remove <alias>`        | Remove an existing alias.                                                |
| `alias reload`                | Reload aliases from `cli_aliases.conf`.                                  |
| `cli set verbose <level>`     | Set the CLI verbosity/logging threshold (0=errors, higher=more verbose). |

### Module-Provided Commands

#### calendar module

| Command                           | Description                                               |
| --------------------------------- | --------------------------------------------------------- |
| `calendar show commands`          | List all calendar-related CLI commands.                   |
| `calendar show events`            | Display all configured calendar sources and their status. |
| `calendar refresh <calendarName>` | Force immediate reload of the named calendar source.      |

#### database module

| Command                                | Description                                                 |
| -------------------------------------- | ----------------------------------------------------------- |
| `database show [<family>]`             | List all database keys or those within a specified family.  |
| `database get <family> <key>`          | Retrieve the value for `<key>` in `<family>`.               |
| `database put <family> <key> <value>`  | Store `<value>` under `<key>` in `<family>`.                |
| `database del <family> <key>`          | Delete `<key>` from `<family>`.                             |
| `database deltree <family> [<prefix>]` | Delete all keys in `<family>` or those matching `<prefix>`. |

#### logger module

| Command                                   | Description                                               |                                                                    |
| ----------------------------------------- | --------------------------------------------------------- | ------------------------------------------------------------------ |
| `logger show commands`                    | List all logger-related CLI commands.                     |                                                                    |
| `logger show channels`                    | List all logging channels and their current levels.       |                                                                    |
| \`logger set level <LEVEL> \<on           | off>\`                                                    | Enable or disable the specified log level on console and channels. |
| `logger mute`                             | Pause all console logging output.                         |                                                                    |
| `logger reload`                           | Reload logger configuration from disk.                    |                                                                    |
| `logger rotate`                           | Rotate open log files without restarting the application. |                                                                    |
| `logger add channel <logfile> <level>`    | Start logging at `<level>` to the given `<logfile>`.      |                                                                    |
| `logger remove channel <logfile> <level>` | Stop logging at `<level>` to the specified `<logfile>`.   |                                                                    |

#### module module

| Command                         | Description                                               |
| ------------------------------- | --------------------------------------------------------- |
| `module show commands`          | List all module-related CLI commands.                     |
| `module load <module-name>`     | Dynamically load the specified module.                    |
| `module unload <module-name>`   | Unload the specified module at runtime.                   |
| `module reload [<module-name>]` | Reload one or all modules.                                |
| `module show [like <pattern>]`  | List loaded modules, optionally filtering by name.        |
| `core reload`                   | Global alias for reloading all modules and configuration. |

#### clipboard module

| Command                   | Description                                        |
| ------------------------- | -------------------------------------------------- |
| `clipboard show commands` | List all clipboard-related CLI commands.           |
| `clipboard show`          | Display the current clipboard contents.            |
| `clipboard save <path>`   | Save the clipboard contents to a file at `<path>`. |
| `clipboard load <path>`   | Load the contents of `<path>` into the clipboard.  |
| `clipboard clear`         | Clear the current clipboard contents.              |

#### opportunity module

| Command                                   | Description                                        |                                         |                                      |
| ----------------------------------------- | -------------------------------------------------- | --------------------------------------- | ------------------------------------ |
| `opportunity show commands`               | List all opportunity-related CLI commands.         |                                         |                                      |
| `opportunity show opps`                   | Show all opportunities for all account executives. |                                         |                                      |
| `opportunity show <ae>`                   | Show all opportunities for the specified AE.       |                                         |                                      |
| `opportunity show <oppname>`              | Display details for a specific opportunity.        |                                         |                                      |
| `opportunity remove <oppname>`            | Delete the specified opportunity.                  |                                         |                                      |
| \`opportunity set <oppname> \<active      | inactive>\`                                        | Mark an opportunity active or inactive. |                                      |
| \`opportunity set status <oppname> \<open | closed-won                                         | closed-lost>\`                          | Update the status of an opportunity. |

#### eventbus module

| Command                                         | Description                               |                                                   |
| ----------------------------------------------- | ----------------------------------------- | ------------------------------------------------- |
| `eventbus show commands`                        | List all eventbus-related CLI commands.   |                                                   |
| `eventbus show events`                          | Display all registered event types.       |                                                   |
| `eventbus show subscribers`                     | List all active event subscribers.        |                                                   |
| \`eventbus subscribe <eventName> \[--sync       | --async]\`                                | Subscribe the CLI session to the specified event. |
| `eventbus unsubscribe <subscriptionId>`         | Cancel a previously created subscription. |                                                   |
| `eventbus publish <eventName> <StructuredData>` | Manually publish an event with a payload. |                                                   |

---

**End of Core: CLI Engine System PRD (v3)**
