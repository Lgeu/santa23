"""
パリティのせいで辺のスコアが悪くなってそうな問題を調べる
"""

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

face_solution_dir = Path("../cpp/solution_face")
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

# 各 order で、edge_score が大きい方から半分を取り出す
df = df.groupby("order").apply(
    lambda df: df.sort_values("edge_score", ascending=False).iloc[: len(df) // 2]
)
for order in range(4, 6):
    print(order)
    print(sorted(df.loc[order]["problem_id"].to_list()))
