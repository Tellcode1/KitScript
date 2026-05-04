fn main()
{
  io::println(io::STDOUT, "hi");
  io::println(io::STDERR, "hi but from stderr");

  let f = io::open("file_with_lines", "r");
  if (!f)
  {
    println("Failed to open file_with_lines");
    return -1;
  }

  let s = null;
  while ((s = io::readln(f)) != null)
  {
    print(s);
  }

  io::close(f);
}