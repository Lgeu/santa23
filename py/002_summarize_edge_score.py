from pathlib import Path

import pandas as pd

dict_order_to_problem_id_range = {
    4: [150, 210],
    5: [210, 245],
    6: [245, 257],
    7: [257, 262],
    8: [262, 267],
    9: [267, 272],
    10: [272, 277],
    19: [277, 281],
    33: [281, 284],
}

edge_solution_dir = Path("../cpp/solution_edge")

data = []
for order, (l, r) in dict_order_to_problem_id_range.items():
    for problem_id in range(l, r):
        with open(edge_solution_dir / f"{problem_id}_best.txt") as f:
            f.readline()
            face_score = int(f.readline())
            f.readline()
            edge_score = int(f.readline().strip())
        data.append([order, problem_id, face_score, edge_score])

df = pd.DataFrame(data, columns=["order", "problem_id", "face_score", "edge_score"])

print(df.groupby("order")[["face_score", "edge_score"]].agg(["count", "mean", "sum"]))
print(df[["face_score", "edge_score"]].sum())
