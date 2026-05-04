fn main() {
  while (true)
  {
    print("Enter the 1st number: ");
    let num1 = float(readln());

    print("Enter the 2nd number: ");
    let num2 = float(readln());

    print("Enter the operation [add/sub/mul/div/pow/sqrt/exit]: ");
    let op = readln();

    let result = null;
    if (str::equal(op, "add\n"))
    {
      result = num1 + num2;
    } else if (str::equal(op, "sub\n"))
    {
      result = num1 - num2;
    } else if (str::equal(op, "mul\n"))
    {
      result = num1 * num2;
    } else if (str::equal(op, "div\n"))
    {
      result = num1 / num2;
    } else if (str::equal(op, "pow\n"))
    {
      result = math::pow(num1, num2);
    } else if (str::equal(op, "sqrt\n"))
    {
      result = math::sqrt(num1);
    } else if (str::equal(op, "exit\n"))
    {
      break;
    } else {
      println("Invalid operation. Continue");
      break;
    }

    println("The result is: ", result);
  }
}