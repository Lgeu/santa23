from pathlib import Path

import numpy as np
import pandas as pd

df = pd.read_csv("../input/puzzles.csv")
df = df[
    df["puzzle_type"].str.startswith("cube_")
    & df["solution_state"].str.startswith("A;A")
]
df.reset_index(drop=True, inplace=True)

out_dir = Path(__file__).resolve()
out_dir = out_dir.with_name(out_dir.name.split("_")[0])
out_dir.mkdir()

for id_, initial_state in df[["id", "initial_state"]].values:
    initial_state = [ord(c) - ord("A") for c in initial_state[::2]]
    initial_state = np.array(initial_state, dtype=np.uint8).reshape(6, -1)
    # Kaggle: UFRBLD
    # Solver: UDRLFB
    initial_state[[0, 5, 2, 4, 1, 3]].tofile(out_dir / f"{id_}")
