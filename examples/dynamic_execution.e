fn main() {
  let fd = io::open("examples/cat.e", "rb");
  defer io::close(fd);

  let file_contents = "";
  for (let line = io::readln(fd); line != null; line = io::readln(fd)) {
    file_contents = str::cat(file_contents, line);
    file_contents = str::cat(file_contents, "\n");
  }

  let exec_info = rt::exec_info(
      /* source_code */ file_contents,
      /* entry_point */ "main",
      /* optimization_level */ 0,
      /* arguments */ [],
      /* command_line_arguments */ ["examples/cat.e"],
  );
  let r = rt::compile_and_exec(exec_info);
  println("we just executed cat.e (presumably) dynamically!!!");
}