fn main()
{
  let args = sys::get_cmd_args();
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

    // why this no work?
    let line = null;
    // for (let line = io::readln(fd); line != null; line = io::readln(fd))
    while (line = io::readln(fd))
    {
      println(line);
    }

    // Flush stdout after each file.
    io::flush(io::STDOUT);
  }
}