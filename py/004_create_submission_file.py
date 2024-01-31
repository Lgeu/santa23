import pandas as pd
import numpy as np
import json
import glob

def main():
    submission_df = pd.read_csv('./submissions/submission_base.csv')
    info = pd.read_csv('../input/puzzle_info.csv')
    test = pd.read_csv('../input/puzzles.csv')
    sample = pd.read_csv('../input/sample_submission.csv', index_col="id")
    test = test.merge(info, on='puzzle_type')

    submission_df["score"] = submission_df["moves"].apply(lambda x: len(x.split(".")))
    
    cube_solution = glob.glob("../cpp/solution_three_order/*_best.txt")
    for path in cube_solution:
        idx = int(path.split("/")[-1].split("_")[0])
        with open(path, "r") as f:
            moves = f.readline().strip()
        this_score = len(moves.split("."))
        # check if the solution is better than the previous one
        if this_score < submission_df.loc[idx, "score"]:
            # print("update", idx, submission_df.loc[idx, "score"], this_score)
            submission_df.loc[idx, "moves"] = moves

    cube_rainbow_solution = glob.glob("../cpp/solution_rainbow/*_best.txt")
    for path in cube_rainbow_solution:
        idx = int(path.split("/")[-1].split("_")[0])
        with open(path, "r") as f:
            moves = f.readline().strip()
        submission_df.loc[idx, "moves"] = moves

    outputs = glob.glob("../cpp/solution_wreath/*_best.txt")
    outputs.sort()
    for path in outputs:
        idx = int(path.split("/")[-1].split("_")[0])
        with open(path, "r") as f:
            moves = f.readline().strip()
        submission_df.loc[idx, "moves"] = moves

    globe_solution = glob.glob("../cpp/solution_globe/*_best.txt")
    for path in globe_solution:
        idx = int(path.split("/")[-1].split("_")[0])
        with open(path, "r") as f:
            moves = f.readline().strip()
        submission_df.loc[idx, "moves"] = moves

    # validation
    total_score = 0
    scores = []
    errors_list = []
    ok = True
    for id in range(0, 398):
        try:
            solution = test.loc[test.id == id, "solution_state"]
            state = test.loc[test.id == id, "initial_state"]
            moves = test.loc[test.id == id, "allowed_moves"]
            wc = test.loc[test.id == id, "num_wildcards"].item()
            solution = solution.item().split(";")
            state = state.item().split(";")
            ans = submission_df.loc[id, "moves"]
            ans = ans.split(".")

            moves = json.loads(moves.item().replace("'", '"'))
            moves_inv = {}
            for key, item in moves.items():
                inv = [0] * len(item)
                for i, m in enumerate(item):
                    inv[m] = i
                moves_inv["-" + key] = inv
            moves_all = moves | moves_inv  

            for a in ans:
                state = [state[m] for m in moves_all[a]]
            score = len(ans)
            total_score += score
            scores.append(score)
            errors = sum([a != b for a, b in zip(state, solution)])
            errors_list.append(errors)
            print(f"testcase id {id} score: {score}, errors: {errors} wc: {wc} check: {errors <= wc}")
            if errors > wc:
                print(f"testcase id {id}: {np.array(state).reshape(4, -1)}")
                print(f"testcase id {id}: {np.array(solution).reshape(4, -1)}")
                print(f"testcase id {id}: {ans}")
                ok = False
        except:
            ok = False
            print(f"testcase id {id}: skipped due to error")
    print(f"total score: {total_score}")
    print(f"validatin result: {ok}")

    submission_df.drop("score", axis=1).to_csv(f"./submissions/submission_{total_score}.csv", index=False)

if __name__ == "__main__":
    main()
