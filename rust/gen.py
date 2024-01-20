# Heuristic Transformer for Kaggle Santa-2023
# Copyright (c) 2024 toast-uz
# Released under the MIT license
# https://opensource.org/licenses/mit-license.php

# 1) Generate input files like a heuristic contest
# Usage: python gen.py -i
# input file format:
#   N M W
#   puzzle_type
#   move_id_1 move_id_2 ... move_id_M
#   solution_state_1 solution_state_2 ... solution_state_N
#   initial_state_1 initial_state_2 ... initial_state_N
#
#   where N is the number of facelets, M is the number of moves, W is the number of wildcards

# 2) Generate submission.csv from output files as a heuristic contest
# Usage: python gen.py -o > submission.csv
# output file format:
#   id,moves
#   where moves is a sequence of move ids separated by periods
#   (moves format are equivalent to the Kaggle's submission.csv)

# 3) Special usage for experts:
# You can leverage the optimal solution from the previous round as the initial input.
# I firmly believe that the best solution from the prior round encapsulates valuable insights for the forthcoming round.
#
# 3-1) Generate output files from submission.csv
# Usage: python gen.py --inverse_o < submission.csv
#
# 3-2) Generate input file merged with output file
# Usage: python gen.py -I
# input file format (additional):
#   moves_1_sequencial_number moves_1_power ...

KAGGLE_DATA_DIR = '../input/'
HEURISTIC_INPUT_DIR = './tools/in'
HEURISTIC_OUTPUT_DIR = './tools/out'

import pandas as pd
import sys
import os
from ast import literal_eval
from itertools import groupby
import glob
import argparse
from collections import Counter

RED = '\033[1m\033[31m'
GREEN = '\033[1m\033[32m'
BLUE = '\033[1m\033[34m'
NORMAL = '\033[0m'

def make_input(puzzles, puzzle_info, merge=False):    
    print(f'{GREEN}Writing {HEURISTIC_INPUT_DIR} ...{NORMAL}', end='', flush=True, file=sys.stderr)
    for _, puzzle in puzzles.iterrows():
        puzzle_id = puzzle['id']
        puzzle_type = puzzle['puzzle_type']
        solution_state = puzzle['solution_state'].split(';')
        facelet_map = {k: i for i, (_, k) in enumerate(
            sorted((ord(k[0]) + int((k + '0')[1:]), k) for k in set(solution_state)))}
        print(f'{GREEN}{facelet_map}{NORMAL}', end='', flush=True, file=sys.stderr)
        solution_state = list(map(lambda k: facelet_map[k], solution_state))
        initial_state = list(map(lambda k: facelet_map[k], puzzle['initial_state'].split(';')))
        num_wildcards = puzzle['num_wildcards']
        allowed_moves = literal_eval(puzzle_info.loc[puzzle_type, 'allowed_moves'])
        move_types_map = {k: i for i, k in enumerate(allowed_moves)}
        allowed_moves = [(k, v) for k, v in allowed_moves.items()]
        assert len(solution_state) == len(initial_state), f'{RED}Solution state and initial state have different lengths{NORMAL}'
        print(f'{GREEN} {puzzle_id:04}.txt{NORMAL}', end='', flush=True, file=sys.stderr)
        # output_fileの読み込み
        if merge:
            with open(f'{HEURISTIC_OUTPUT_DIR}/{puzzle_id:04}.txt', 'r') as f:
                moves = f.readline().rstrip()
            move_types = moves.split('.')
            moves = []
            for move_type in move_types:
                power = -1 if move_type[0] == '-' else 1
                move_type = move_type.removeprefix('-')
                try:
                    moves.append((move_types_map[move_type], power))
                except KeyError:
                    print(f'{RED}Invalid move id: {move_type}{NORMAL}', file=sys.stderr)
                    exit(1)
        # input_fileの書き込み
        with open(f'{HEURISTIC_INPUT_DIR}/{puzzle_id:04}.txt', 'w') as f:
            f.write(f'{len(solution_state)} {len(allowed_moves)} {num_wildcards}\n')
            f.write(f'{puzzle_type}\n')
            f.write(f'{" ".join(move_id for move_id, _ in allowed_moves)}\n')
            f.write(f'{" ".join(map(str, solution_state))}\n')
            f.write(f'{" ".join(map(str, initial_state))}\n')
            for _, perm in allowed_moves:
                f.write(f'{" ".join(map(str, perm))}\n')
            if merge:
                f.write(f'{len(moves)}\n')
                f.write(f'{" ".join(map(lambda m: f"{m[0]} {m[1]}", moves))}')
    print(f'{GREEN}Done{NORMAL}', flush=True, file=sys.stderr)

def make_submission():
    res = pd.DataFrame(columns=['id', 'moves'])
    for output_file in glob.glob(f'{HEURISTIC_OUTPUT_DIR}/*.txt'):
        id = int(os.path.splitext(os.path.basename(output_file))[0])
        with open(output_file, 'r') as f:
            moves = f.readline().rstrip()
        res = pd.concat([res, pd.DataFrame([[id, moves]], columns=['id', 'moves'])])
    res = res.sort_values('id')
    return res

def make_output(submission):
    print(f'{GREEN}Writing {HEURISTIC_OUTPUT_DIR} ...{NORMAL}', end='', flush=True, file=sys.stderr)
    for _, row in submission.iterrows():
        id = row['id']
        moves = row['moves']
        with open(f'{HEURISTIC_OUTPUT_DIR}/{id:04}.txt', 'w') as f:
            f.write(f'{moves}\n')
    print(f'{GREEN}Done{NORMAL}', flush=True, file=sys.stderr)

def main():
    for dir_ in [KAGGLE_DATA_DIR, HEURISTIC_INPUT_DIR, HEURISTIC_OUTPUT_DIR]:
        assert os.path.isdir(dir_), f'{RED}{dir_} is not a directory{NORMAL}'
    args = argparse.ArgumentParser()
    args.add_argument('-i', '--input', action='store_true')
    args.add_argument('-I', '--input_merged_output', action='store_true')
    args.add_argument('-o', '--output', action='store_true')
    args.add_argument('--inverse_o', action='store_true')
    args = args.parse_args()
    if sum([args.input, args.input_merged_output, args.output, args.inverse_o]) != 1:
        print('Usage: python gen.py -i')
        print('Usage: python gen.py -o > submission.csv')
        exit(1)

    if args.input or args.input_merged_output:
        puzzles = pd.read_csv(f'{KAGGLE_DATA_DIR}/puzzles.csv')
        puzzle_info = pd.read_csv(f'{KAGGLE_DATA_DIR}/puzzle_info.csv', index_col='puzzle_type')
        make_input(puzzles, puzzle_info, args.input_merged_output)
    elif args.output:
        submission = make_submission()
        submission.to_csv(sys.stdout, index=False)
    elif args.inverse_o:
        submission = pd.read_csv(sys.stdin)
        make_output(submission)

if __name__ == '__main__':
    main()
