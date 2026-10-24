fn disallow_optimizations(var) {
  let v = var;
  var = v; /* Not an actual write back, just stop the optimizer */
  return;
}

fn main() {

  const iterations = 100000;

  let vectors = [69];

  let start = time::mono();
  let sum = vec2::ZERO;
  for (let i = 0; i < iterations; i++) {
    sum += vectors[0];
    disallow_optimizations(sum);
  }

  let end = time::mono();
  println("Took ", end - start, " seconds");
}