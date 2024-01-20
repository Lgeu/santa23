// Heuristic Transformer for Kaggle Santa-2023
// Copyright (c) 2024 toast-uz
// Released under the MIT license
// https://opensource.org/licenses/mit-license.php

// Usage: cargo run --release --bin vis <input> <output>

use proconio::{input, source::auto::AutoSource};
use rustc_hash::FxHashMap as HashMap;

fn main() {
    let args: Vec<_> = std::env::args().collect();
    if args.len() != 3 {
        eprintln!("Usage: {} <input> <output>", args[0]);
        std::process::exit(1);
    }
    let e = Env::new(&args[1]);
    let a = Agent::new(&e, &args[2]);
    if !a.validate(&e) {
        std::process::exit(1);
    }
    eprintln!("Score = {}", a.compute_score());
}

// struct Env is used for reading the input environment.

#[allow(dead_code)]
#[derive(Debug)]
struct Env {
    n: usize, m: usize, w: usize,
    puzzle_type: String,
    move_types: Vec<String>,
    solution_state: Vec<usize>,
    initial_state: Vec<usize>,
    allowed_moves: Vec<SparsePermutation>,
}

impl Env {
    fn new(input_file: &str) -> Self {
        let input_str = std::fs::read_to_string(input_file).unwrap();
        let input = AutoSource::from(&input_str[..]);
        input! {
            from input,
            n: usize, m: usize, w: usize,
            puzzle_type: String,
            move_types: [String; m],
            solution_state: [usize; n],
            initial_state: [usize; n],
            allowed_moves: [[usize; n]; m],
        }
        let allowed_moves = allowed_moves.iter()
            .map(|x| SparsePermutation::new(x)).collect();
        Self { n, m, w, puzzle_type, move_types,
            solution_state, initial_state, allowed_moves }
    }
}

// struct Agent is used for optimizing the solution.

struct Agent {
    moves: Vec<(usize, isize)>,  // (move_id, power))    
}

impl Agent {
    // construct an agent from the output file
    fn new(e: &Env, output_file: &str) -> Self {
        let move_types_map: HashMap<String, usize> = e.move_types.iter()
            .enumerate().map(|(i, s)| (s.clone(), i)).collect();
        let move_types = std::fs::read_to_string(output_file)
            .unwrap().trim().to_string();
        let move_types = move_types.split('.');
        let mut moves = Vec::new();
        for move_type in move_types {
            let (move_type, power) =
                if let Some(move_type) = move_type.strip_prefix('-') {
                    (move_type, -1) } else { (move_type, 1) };
            let Some(&move_id) = move_types_map.get(&move_type.to_string()) else {
                eprintln!("Invalid move_id: {}", move_type);
                std::process::exit(1);
            };
            moves.push((move_id, power));
        }
        Self { moves }
    }
    // validate the solution
    fn validate(&self, e: &Env) -> bool {
        let mut state = e.initial_state.clone();
        for &(move_id, power) in &self.moves {
            let move_ = &e.allowed_moves[move_id];
            move_.apply_inplace(&mut state, power);
        }
        let num_wrong_facelets = state.wrong_metric(&e.solution_state);
        if num_wrong_facelets > e.w {
            eprintln!("Too many wrong facelets: {} > num_wildcards: {}", num_wrong_facelets, e.w);
            return false;
        }
        true
    }
    // compute the score
    fn compute_score(&self) -> usize {
        self.moves.len()
    }
}


// supporting structs and traits

/* SparsePermutation
    Compatible with Python SymPy's Permutation.
    Efficient for large permutations with sparse transpositions.

    Example 1:
    `Self::new(&[1, 2, 0])` represents the permutation (0, 1, 2) -> (1, 2, 0).
    The item of element 0 is transferred to the item of element 1,
    the item of element 1 is transferred to the item of element 2,
    and the item of element 2 is transferred to the item of element 0.

    let perm = SparsePermutation::new(&[1, 2, 0]);
    let x = vec!["A", "B", "C"];    // able to use any type with Clone trait
    assert_eq!(perm.apply(&x, 1), vec!["B", "C", "A"]);

    Example 2:
    The power parameter is used to repeat the permutation multiple times.
    (A negative power is used to apply the inverse permutation.)
    let perm = SparsePermutation::new(&[1, 2, 0]);
    let x = vec!["A", "B", "C"];    // able to use any type with Clone trait
    assert_eq!(perm.apply(&x, 2), vec!["C", "A", "B"]);
    assert_eq!(perm.apply(&x, -1), vec!["C", "A", "B"]);
*/

#[derive(Debug, Clone)]
pub struct SparsePermutation {
    // (from, to) : the elm of the index from is transfeerd to the elm of the index to
    from_to: Vec<(usize, usize)>,
}

impl SparsePermutation {
    pub fn len(&self) -> usize { self.from_to.len() }
    pub fn new(perm: &[usize]) -> Self {
        let from_to = (0..perm.len())
            .filter(|&i| i != perm[i])  // make sparse
            .map(|i| (i, perm[i])).collect();
        Self { from_to }
    }
    // apply the permutation to the vector x
    pub fn apply<T: Clone>(&self, x: &[T], power: isize) -> Vec<T> {
        let mut res = x.to_vec();
        self.apply_inplace(&mut res, power);
        res
    }
    // apply the permutation to the vector x in place
    pub fn apply_inplace<T: Clone>(&self, x: &mut [T], power: isize) {
        let sig = power.signum();
        let power_norm = power.abs() as usize;
        for _ in 0..power_norm {
            if sig == 1 {
                //　the elm of the index from is transfeerd to the elm of the index to
                let elm_of_to = (0..self.len())
                    .map(|i| x[self.from_to[i].1].clone()).collect::<Vec<_>>();
                for i in 0..self.len() { x[self.from_to[i].0] = elm_of_to[i].clone(); }
            } else {
                // the elm of the index to is transfeerd to the elm of the index from
                let elm_of_from = (0..self.len())
                    .map(|i| x[self.from_to[i].0].clone()).collect::<Vec<_>>();
                for i in 0..self.len() { x[self.from_to[i].1] = elm_of_from[i].clone(); }
            }
        }
    }
}

// Trait for measuring dissimilarity between two instances.

trait WrongMetric {
    fn wrong_metric(&self, other: &Self) -> usize;
}

impl<T: PartialEq> WrongMetric for [T] {
    fn wrong_metric(&self, other: &Self) -> usize {
        (0..self.len()).filter(|&i| self[i] != other[i]).count()
    }
}
