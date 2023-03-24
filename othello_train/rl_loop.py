import argparse
import subprocess
from pathlib import Path
from typing import Optional


def check_call(args, skip_if_exists: Optional[Path] = None):
    if skip_if_exists is not None and skip_if_exists.exists():
        print("#skip: " + " ".join(args))
        return
    print(" ".join(args))
    subprocess.check_call(args)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("work_dir")
    parser.add_argument("--epoch", type=int, default=10)
    parser.add_argument("--games", type=int, default=10000)
    args = parser.parse_args()

    work_dir = Path(args.work_dir)
    records_dir = work_dir / "records"
    records_dir.mkdir(parents=True, exist_ok=True)

    for epoch in range(args.epoch):
        if epoch == 0:
            check_call(["python", "-m", "othello_train.make_empty_model_v1",
                       f"{work_dir}/cp_{epoch}/cp"], work_dir / f"cp_{epoch}")
            check_call(["python", "-m", "othello_train.checkpoint_to_savedmodel_v1",
                        f"{work_dir}/cp_{epoch}/cp", f"{work_dir}/sm_{epoch}"], work_dir / f"sm_{epoch}")
        check_call(["python", "-m", "othello_train.playout_v1",
                   f"{work_dir}/sm_{epoch}", f"{records_dir}/records_{epoch}.bin", "--games", f"{args.games}"], records_dir / f"records_{epoch}.bin")
        check_call(["python", "-m", "othello_train.rl_train_v1", f"{work_dir}/cp_{epoch}/cp", f"{work_dir}/cp_{epoch+1}/cp",
                   f"{records_dir}/records_{epoch}.bin"], work_dir / f"cp_{epoch+1}")
        check_call(["python", "-m", "othello_train.checkpoint_to_savedmodel_v1",
                   f"{work_dir}/cp_{epoch+1}/cp", f"{work_dir}/sm_{epoch+1}"], work_dir / f"sm_{epoch+1}")


if __name__ == "__main__":
    main()