fn io_type_to_string(type)
{
  if (type == io::FILE)
  {
    return "io::FILE";
  } else if (type == io::DIRECTORY)
  {
    return "io::DIRECTORY";
  } else if (type == io::LINK)
  {
    return "io::LINK";
  } else
  {
    return "io::UNKNOWN";
  }
}

fn main() {
  let f_type = io::type("build");
  println("Typeof(build) = ", io_type_to_string(f_type));

  f_type = io::type("arena.h");
  println("Typeof(arena.h) = ", io_type_to_string(f_type));
}