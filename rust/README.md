**Heuristic Transformer for Kaggle Santa-2023**

For all Kagglers who love heuristic contests! Introducing a specialized tool designed for the essence of heuristic contests, tailored for Santa-2023. This tool is crafted with three main features to enhance your experience:

**Key Features:**

1. **Kaggle Data Transformation:**
   - Convert Kaggle dataset into heuristic input, providing a problem setup in a familiar format.
   
2. **Evaluation and Scoring:**
   - Evaluate the accuracy of heuristic input-output pairs with lightning-fast speed, generating scores to determine the precision of the output. Experience the thrill of competition with instant feedback.

3. **Submission.csv Conversion:**
   - Transform heuristic outputs into an easily usable format.

**How to Use:**

1. **Kaggle Data Transformation:**
   - Import Kaggle dataset into the tool and observe its transformation into heuristic input.

   Usage: `python gen.py -i`

   Example of a heuristic input (in/0000.txt):
   ```
   24 6 0
   cube_2/2/2
   f0 f1 r0 r1 d0 d1
   0 0 0 0 1 1 1 1 2 2 2 2 3 3 3 3 4 4 4 4 5 5 5 5
   3 4 3 0 4 1 0 1 2 0 2 0 3 2 3 5 5 5 4 4 1 5 1 2
   0 1 19 17 6 4 7 5 2 9 3 11 12 13 14 15 16 20 18 21 10 8 22 23
   18 16 2 3 4 5 6 7 8 0 10 1 13 15 12 14 22 17 23 19 20 21 11 9
   0 5 2 7 4 21 6 23 10 8 11 9 3 13 1 15 16 17 18 19 20 14 22 12
   4 1 6 3 20 5 22 7 8 9 10 11 12 2 14 0 17 19 16 18 15 21 13 23
   0 1 2 3 4 5 18 19 8 9 6 7 12 13 10 11 16 17 14 15 22 20 23 21
   1 3 0 2 16 17 6 7 4 5 10 11 8 9 14 15 12 13 18 19 20 21 22 23
   ```

2. **Evaluation and Scoring:**
   - Provide heuristic input-output pairs for evaluation. Receive scores instantly to gauge the accuracy of your output.

   Usage: `cargo run --release --bin vis <input> <output>`

3. **Submission.csv Generation:**
   - Save heuristic outputs in a convenient format for future use.

   Usage: `python gen.py -o > submission.csv`

   Example of a heuristic output (out/0000.txt):
   ```
   r1.-f1
   ```
   