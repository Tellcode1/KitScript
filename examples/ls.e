fn main() {
  let args = sys::get_cmd_args();

  for (let i = 0; i < len(args); i++) {
    const f = args[i];
    
    if (io::type(f) == io::DIRECTORY) println(io::listdir(f));
    
    else if (io::type(f) == io::FILE) println(f);

    else println("Failed to open ", f, ": ", io::error());
  }
}