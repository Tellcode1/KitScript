fn hello(a) {
  println(a);
}

fn main() {
  hello(32);
  println("multifile_first calling our cohort");
  println(hello_from_another_file_2());
}