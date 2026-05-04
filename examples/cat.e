fn main()
{
  let args = sys::get_command_line_args();
  if (list::len(args) == 0) return;

  for (let i = 0; i < list::len(args); i++)
  {
    let fd = io::open(args[i], "r");
    if (fd == null)
    {
      io::println(io::STDERR, "cat: Failed to open file: ", args[i]);
      io::println(io::STDERR, "cat: ", io::error());
      continue;
    }

    defer io::close(fd);

    let s = null;
    while (s = io::readln(fd))
    {
      print(s);
    }

    // Flush stdout after each file.
    io::flush(io::STDOUT);
  }
}