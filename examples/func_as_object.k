const OPERATE_ADD = 0;
const OPERATE_SUB = 1;
const OPERATE_MUL = 2;

fn add(x,y) { return x + y; }
fn sub(x,y) { return x - y; }
fn mul(x,y) { return x * y; }

fn operate_function(operation) {
  if (operation == OPERATE_ADD) {
    return add;
  }
  if (operation == OPERATE_SUB) {
    return sub;
  }
  if (operation == OPERATE_MUL) {
    return mul;
  }
}

fn even() {
  println("Even");
}
fn odd() {
  println("Odd");
}

fn repeat_this() {
  println("repeated");
}

fn repeat(f, n) {
  for (i in 0..n) {
    f();
  }
}

fn main() {
  println(operate_function(OPERATE_ADD)(10, 20));

  let x = [even, odd];
  for (i in 2..11) {
    print(i, " is ");
    x[i % 2]();
  }

  repeat(repeat_this, 5);
}