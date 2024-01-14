mkdir bin log log/kaggle solution_face

make clean
make all -j


./bin/face_formula 4 7 &
./bin/face_formula 4 8 &
./bin/face_formula 5 7 &
./bin/face_formula 5 8 &
./bin/face_formula 6 7 &
./bin/face_formula 6 8 &
./bin/face_formula 7 7 &
./bin/face_formula 7 8