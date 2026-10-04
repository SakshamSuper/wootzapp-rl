# WootzApp RL Browser Environment

## Overview

This repository contains a working milestone for the WootzApp Data Science / RL task. The project builds a small browser-based shopping environment, "MiniShop", and connects it to an external agent loop through a C++ environment process.

The implemented system demonstrates:

- A MiniShop web page with catalog, product, cart, checkout, popup, delay, and URL goal hooks.
- Direct Chrome DevTools Protocol (CDP) communication from C++ over a WebSocket.
- Environment-style `reset` and `step` behavior.
- Supported actions for visible button clicks and waiting.
- Real browser mouse events through CDP `Input.dispatchMouseEvent`.
- A Python random agent that communicates with the C++ environment over standard input.
- JSONL episode logging for the random agent.

This is not a complete RL evaluation submission yet. Q-learning, full multi-goal evaluation, statistical analysis, learning curves, and the final report are still pending.

## Architecture

The current architecture is:

```text
Python random agent
        |
        | stdin actions: click:0, click:1, click:2, wait, quit
        v
C++ MiniShop environment
        |
        | Chrome DevTools Protocol over WebSocket
        v
Chrome running MiniShop
```

Main files:

- `site/index.html` implements the MiniShop browser task.
- `environment/environment.cpp` implements the C++ environment wrapper.
- `agents/random_agent.py` runs a random baseline episode against the C++ environment.
- `random_results.jsonl` stores JSONL episode logs produced by the random agent.

## Implementation

### MiniShop

`site/index.html` contains a small shopping flow:

- Product catalog.
- Product detail screen.
- Quantity controls.
- Cart screen.
- Checkout completion screen.
- Optional popup controlled by `popup_p`.
- Optional delay hook controlled by `delay_p`.
- URL goal parameters such as `item` and `qty`.

### C++ Environment

`environment/environment.cpp` connects directly to Chrome on `localhost:9222` using WinHTTP and a WebSocket CDP endpoint.

Implemented behavior:

- `reset()` opens the Chrome CDP WebSocket connection and reads the current page text as the initial observation.
- `step("wait")` waits briefly, then observes the page again.
- `step("click:i")` finds visible buttons in the page, selects button index `i`, computes its center coordinates, and sends real mouse press/release events through CDP.
- Observations are printed from `document.body.innerText`.

Currently supported actions:

```text
click:0
click:1
click:2
wait
```

### Python Random Agent

`agents/random_agent.py`:

- Starts `environment/environment.exe`.
- Samples random actions from `click:0`, `click:1`, `click:2`, and `wait`.
- Sends actions to the C++ process through standard input.
- Stops the environment with `quit`.
- Appends one JSON record per episode to `random_results.jsonl`.

The current JSONL record includes:

- `agent`
- `steps`
- `actions`
- `elapsed_seconds`

## How to Run

This project currently targets Windows because the C++ environment uses WinHTTP and Windows headers.

1. Build the C++ environment.

   From the repository root, using a Visual Studio Developer Command Prompt:

   ```bat
   cl /EHsc environment\environment.cpp /Fe:environment\environment.exe winhttp.lib
   ```

2. Start Chrome with remote debugging enabled on port `9222`.

   Example:

   ```bat
   chrome.exe --remote-debugging-port=9222 --user-data-dir="%TEMP%\wootzapp-rl-chrome"
   ```

3. Open MiniShop in that Chrome instance.

   Open `site/index.html` in the remote-debugging Chrome window.

4. Confirm the CDP target.

   The current C++ file uses a verified hard-coded page WebSocket path. If Chrome creates a different target id, the path in `environment/environment.cpp` must be updated and the environment rebuilt.

5. Run the random agent.

   From the repository root:

   ```bat
   python agents\random_agent.py
   ```

6. Check the JSONL log.

   The random agent appends episode data to:

   ```text
   random_results.jsonl
   ```

## Evaluation / Current Results

Current verified status:

- MiniShop page exists and supports the shopping flow.
- Chrome CDP connection from C++ over WebSocket has been implemented.
- `reset()` obtains an observation from the browser page.
- `step()` supports `wait` and real mouse clicks on visible buttons.
- Python can start the C++ environment and send actions to it.
- The random agent can run an episode and append a JSONL log.

No final numerical evaluation results are claimed in this repository yet.

The existing JSONL logging shows random-agent episode metadata, but it should not be treated as a complete benchmark or RL result.

## Limitations & Pending Work

The following task requirements are pending or incomplete:

- Q-learning baseline.
- Full 12-goal x 3-seed evaluation.
- Popup probability experiments.
- Confidence intervals or error intervals.
- Learning curves.
- Full evaluation report.

Known implementation limitations:

- The C++ environment currently uses a hard-coded Chrome page WebSocket path.
- The action space is limited to `click:0`, `click:1`, `click:2`, and `wait`.
- Observations are currently page text from `document.body.innerText`.
- JSONL logging records random-agent episode metadata, but does not yet include a robust success metric.
- The current agent is random and does not learn.

## Submission Notes

This repository should be evaluated as a working integration milestone, not as a completed RL benchmark.

Implemented and verified components include MiniShop, direct Chrome CDP communication through C++/WebSocket, environment reset/step behavior, wait and real mouse click actions, Python-to-C++ communication, a Python random agent, and JSONL logging.

The incomplete evaluation items are explicitly listed above and should be addressed before claiming final RL performance.
