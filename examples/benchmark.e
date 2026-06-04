fn disallow_optimizations(var) {
  let v = var;
  var = v; /* Not an actual write back, just stop the optimizer */
  return;
}

fn main() {
  let start = time::mono();

  const iterations = 1000000;

  let vectors = [];
  for (let i = 0; i < iterations; i++) {
    list::append(vectors, rand::vec2());
  }

  let sum = vec2::ZERO;
  for (let i = 0; i < iterations; i++) {
    sum += vectors[i];
    disallow_optimizations(sum);
  }


  let end = time::mono();
}