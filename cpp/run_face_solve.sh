# run ./bin/face_solve_7_7 {input1} {input2} | tee log/kaggle/

if [ $# -ne 3 ]; then
  echo "Usage: $0 {problem_id} {initial_beam_width} {n_threads}"
  exit 1
fi

depth=8
id=$1
n_threads=$3

# change order according to problem_id
if [ $id -lt 150 ]; then
    echo "id has to be greater than 150"
    exit 1
elif [ $id -lt 200 ]; then
    order=4
    mode="normal"
elif [ $id -lt 210 ]; then
    order=4
    mode="rainbow"
elif [ $id -lt 240 ]; then
    order=5
    mode="normal"
elif [ $id -lt 245 ]; then
    order=5
    mode="rainbow"
elif [ $id -lt 255 ]; then
    order=6
    mode="normal"
elif [ $id -lt 257 ]; then
    order=6
    mode="rainbow"
elif [ $id -lt 262 ]; then
    order=7
    mode="normal"
elif [ $id -lt 267 ]; then
    order=8
    mode="normal"
elif [ $id -lt 272 ]; then
    order=9
    mode="normal"
elif [ $id -lt 277 ]; then
    order=10
    mode="normal"
elif [ $id -lt 281 ]; then
    order=19
    mode="normal"
elif [ $id -lt 283 ]; then
    order=33
    mode="normal"
elif [ $id -lt 284 ]; then
    order=33
    mode="rainbow"
else
    echo "id has to be less than 283"
    exit 1
fi


echo "problem id = $id"
echo "Running face_solve_${order}_${depth}_${mode} ${1} ${2} ${3}"

./bin/face_solve_${order}_${depth}_${mode} ${1} ${2} ${3} | tee log/kaggle/${id}.txt