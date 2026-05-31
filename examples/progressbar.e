
fn progressbar(i, max) {
  let BAR_START = '[';
  let BAR_END = ']';
  let PROGRESS_CHAR = '#';
  let EMPTY_CHAR = 'x';

  println(i, " << >> ", max);
  print(BAR_START);
  for (let j = 0; j < max; j++) {
    if (j < i) print(PROGRESS_CHAR);
    else print(EMPTY_CHAR);
  }
  print(BAR_END, int((float(i) / float(max)) * 100.0), "%\n");
}

fn main() {
  const count = 20;
  for (let i = 0; i <= count; i++) {
    progressbar(i, count);
  }
}