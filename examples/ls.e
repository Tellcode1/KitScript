fn is_flag(s) {
  return s[0] == '-';
}

fn print_fname(show_hidden, fname) {
  if (is_flag(fname)) return;
  if (!show_hidden && (fname[0] == '.')) return;
  println(fname);
}

fn main() {
  let args = sys::get_cmd_args();
  let show_hidden = list::exists(args, "-sh") || list::exists(args, "--show-hidden");

  for (let i = 0; i < len(args); i++) {
    if (is_flag(args[i])) continue;

    if (io::type(args[i]) == io::DIRECTORY) {
      let ents = io::listdir(args[i]);

      for (let j = 0; j < len(ents); j++) {
        print_fname(show_hidden, ents[j]);
      }
    }
    else if (io::exists(args[i])) {
      print_fname(show_hidden, args[i]);
    } else {
      io::println(io::STDERR, "Failed to open ", args[i]);
    }
  }
}