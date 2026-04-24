# Development Guidelines

## Branch Policy

- Never commit directly to `main` / `master`
- All development must start on a dedicated branch:
  - New features: `feature/description`, e.g. `feature/mqtt-client`
  - Bug fixes: `fix/description`, e.g. `fix/adc-timeout`
- After completing and committing, ask the user whether to merge back into main

## Coding Standards

### File Organisation
- Every independent module must have its own `.c` and `.h` file
- Never mix unrelated functionality in the same file
- When adding a major new feature, create a new file — do not append to an existing one

### Function Naming
- Format: `module_action`, all lowercase, words separated by underscores
- Examples:
  - `uart_init()` — UART module initialisation
  - `mqtt_publish()` — MQTT module publish
  - `relay_set()` — relay module set state
- Never use vague generic names such as `init()`, `process()`, or `handle()`

### Decoupling
- Modules communicate through their public interfaces only — never access another module's internal variables directly
- All internal variables must be declared `static` and not exposed externally
- Header files expose only the necessary public interface; implementation details stay in `.c`

### Comments
- Every function must have a doc-comment describing its purpose, parameters, and return value
- Add inline comments on non-obvious logic lines explaining *why*, not just *what*
- Each module file must have a header comment summarising its purpose and dependencies
- Stub functions must be marked with `TODO` and a description of the expected implementation

### Embedded Timeout Policy
- **Never use `portMAX_DELAY` or any form of indefinite blocking**
- All blocking calls must have a finite timeout and handle the timeout return value:
  - `xEventGroupWaitBits()` → use `pdMS_TO_TICKS(N)` instead of `portMAX_DELAY`
  - `xQueueReceive()` / `xSemaphoreTake()` → same rule
  - Spin-wait loops → must have a counter limit
- On timeout: log an error and take a fallback path (skip, retry, or return error) — never hang
- Example:
  ```c
  /* Wrong: indefinite block */
  xEventGroupWaitBits(group, BIT0, pdFALSE, pdFALSE, portMAX_DELAY);

  /* Correct: finite timeout + fallback */
  EventBits_t bits = xEventGroupWaitBits(group, BIT0, pdFALSE, pdFALSE, pdMS_TO_TICKS(5000));
  if (!(bits & BIT0)) {
      ESP_LOGE(TAG, "wait timeout, fallback");
  }
  ```

### Miscellaneous
- All header files must have include guards:
  ```c
  #ifndef MODULE_NAME_H
  #define MODULE_NAME_H
  // ...
  #endif /* MODULE_NAME_H */
  ```

## Commit Convention

- Each commit does exactly one thing
- Commit message format: `type: short description`
  - `feat: add mqtt client module`
  - `fix: resolve sensor read timeout`
  - `refactor: split light control into separate module`
  - `docs: update readme with mqtt setup`
- Messages must be in **English**, start with a verb, all lowercase
