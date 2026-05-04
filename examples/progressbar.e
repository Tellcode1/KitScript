const BAR_START = '[';
const BAR_END = ']';
const PROGRESS_CHAR = '#';
const EMPTY_CHAR = 'x';

fn progressbar(i, max) {
  print(BAR_START);
  for (let j = 0; j < max; j++) {
    if (j < i) print(PROGRESS_CHAR);
    else print(EMPTY_CHAR);
  }
  print(BAR_END, int((float(i) / float(max)) * 100.0), "%\n");
}

fn main() {
  let count = 5;
  for (let j = 1; j <= count; j++) {
    for (let i = 0; i <= count; i++) {
      progressbar(i, j);
    }
  }
}