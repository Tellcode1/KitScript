fn main() {
  let args = sys::get_command_line_args();
  if (len(args) < 2)
  {
    println("Expected expression");
    return -1;
  }

  let x = args[0];
  let op = args[1];

  if (str::equal(x, "sqrt"))
  {
    if (len(args) < 2)
    {
      println("Expected argument");
      return null;
    }
    let r = math::sqrt(float(op));
    println(r);
    return null;
  }
  
  if (len(args) < 3)
  {
    println("Expected argument");
    return null;
  }

  if (str::equal(x, "pow"))
  {
    let x = float(args[1]);
    let y = float(args[2]);
    println(math::pow(x,y));
    return null;
  }


  let r = null;
  if (str::equal(op, "+"))
  {
    r = float(x) + float(y);
  } else if (str::equal(op, "-"))
  {
    r = float(x) - float(y);
  } else if (str::equal(op, "*"))
  {
    r = float(x) * float(y);
  } else if (str::equal(op, "/"))
  {
    r = float(x) / float(y);
  }
  println(r);
  return null;
}