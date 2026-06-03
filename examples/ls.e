fn main() {
  let args = sys::get_cmd_args();

  let lol = [];
  for (let i = 0; i < len(args); i++) {
    const f = args[i];
    
    if (io::type(f) == io::DIRECTORY) println(io::listdir(f));
    
    else if (io::type(f) == io::FILE) println(f);

<<<<<<< Updated upstream
    else println("Failed to open ", f, ": ", io::error());
=======
    if (io::type(args[i]) == io::DIRECTORY) {
      let ents = io::listdir(args[i]);

      for (let j = 0; j < len(ents); j++) {
        if (is_flag(ents[j])) continue;
        if (!show_hidden && (ents[j][0] == '.')) continue;
        list::append(lol, ents[j]);
      }
    }
    else if (io::exists(args[i])) {
      if (is_flag(args[i])) continue;
      if (!show_hidden && (args[i][0] == '.')) continue;
      list::append(lol, args[i]);
    } else {
      io::println(io::STDERR, "Failed to open ", args[i]);
    }
>>>>>>> Stashed changes
  }

  println(lol);
}