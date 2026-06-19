fn open(path,mode) {
  let fd = io::open(path, mode);
  io::println(io::STDOUT, "Opened file: ", path, " with mode: ", mode);
  io::println(io::STDOUT, "Recieved file descriptor: ", fd);
  return fd;
}

fn close(fd) {
  io::println(io::STDOUT, "Closed file: ", fd);
  io::close(fd);
}

fn main()
{
  io::println(io::STDOUT, "hi");
  io::println(io::STDERR, "hi but from stderr");

  const file = "file_with_lines";

  let f = open(file, "r");
  if (f) defer close(f);

  if (!f)
  {
    println("Failed to open ", file, ": ", io::error());
    return -1;
  }

  let s = null;
  while ((s = io::readln(f)) != null)
  {
    println(s);
  }
}