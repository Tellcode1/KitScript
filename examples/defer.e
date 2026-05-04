let ratio = 0;

fn open()
{
  ratio++;
  return io::open("file", "r");
}

fn close(fd)
{
  ratio--;
  io::close(fd);
}

fn main()
{
  for (let i = 0; i < 1000; i++)
  {
    /**
      * Defers in a loop are instantiated each time
      * the loop runs, and will be executed after the loop
      * body but before the iterator updates.
      */
    let fd = open();
    defer close(fd);

    // oo bracketed defers oo
    defer {
      let fd2 = open();
      close(fd2);
    };
  }
  println(ratio, " Files were leaked");
  if (ratio != 0) {
    println("Some files were leaked! There's a bug in defer instantiation (report!)");
  }
}