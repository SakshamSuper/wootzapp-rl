import random
import subprocess
import time
import json
import os


MAX_STEPS = 10

ACTIONS = [
    "click:0",
    "click:1",
    "click:2",
    "wait",
]


def choose_action():
    """Choose a random action."""
    return random.choice(ACTIONS)


def save_log(actions, elapsed):
    """Save episode information as JSONL."""

    record = {
        "agent": "random",
        "steps": len(actions),
        "actions": actions,
        "elapsed_seconds": round(elapsed, 4),
    }

    with open(
        "random_results.jsonl",
        "a",
        encoding="utf-8"
    ) as file:

        file.write(
            json.dumps(record) + "\n"
        )


def main():

    print("================================")
    print("        RANDOM AGENT")
    print("================================")


    # Always start from the project root.
    project_root = os.path.dirname(
        os.path.dirname(
            os.path.abspath(__file__)
        )
    )


    environment_path = os.path.join(
        project_root,
        "environment",
        "environment.exe"
    )


    print(
        "Environment:",
        environment_path
    )


    if not os.path.exists(environment_path):

        print(
            "ERROR: environment.exe not found."
        )

        return


    # --------------------------------------------------
    # Start C++ environment
    # --------------------------------------------------

    process = subprocess.Popen(
        [environment_path],
        stdin=subprocess.PIPE,
        stdout=None,
        stderr=None,
        text=True,
        encoding="utf-8"
    )


    print(
        "C++ environment started."
    )


    time.sleep(2)


    # --------------------------------------------------
    # Check whether C++ is still running
    # --------------------------------------------------

    if process.poll() is not None:

        print(
            "ERROR: C++ environment exited early."
        )

        print(
            "Exit code:",
            process.returncode
        )

        return


    actions = []

    start_time = time.time()


    # --------------------------------------------------
    # Run random episode
    # --------------------------------------------------

    for step in range(MAX_STEPS):

        action = choose_action()

        actions.append(action)


        print(
            f"Step {step + 1}/{MAX_STEPS}: {action}"
        )


        # Check process before sending action.

        if process.poll() is not None:

            print(
                "C++ environment stopped unexpectedly."
            )

            break


        try:

            process.stdin.write(
                action + "\n"
            )

            process.stdin.flush()

        except (OSError, ValueError) as error:

            print(
                "Could not send action to C++:"
            )

            print(error)

            break


        time.sleep(1)


    # --------------------------------------------------
    # Stop environment
    # --------------------------------------------------

    if process.poll() is None:

        try:

            process.stdin.write(
                "quit\n"
            )

            process.stdin.flush()

        except (OSError, ValueError):

            pass


    # Wait briefly for C++ to exit.

    try:

        process.wait(timeout=5)

    except subprocess.TimeoutExpired:

        process.terminate()


    elapsed = time.time() - start_time


    # --------------------------------------------------
    # Save results
    # --------------------------------------------------

    save_log(
        actions,
        elapsed
    )


    print()
    print("================================")
    print("Episode finished")
    print("================================")

    print(
        "Actions:",
        len(actions)
    )

    print(
        f"Time: {elapsed:.2f} seconds"
    )

    print(
        "Log: random_results.jsonl"
    )


if __name__ == "__main__":

    main()