mkdir tools tools/in tools/out
cargo build --release --bin vis
python gen.py -i
