fn v2add(va, vb)
{
  return vec2(va.x + vb.x, va.y + vb.y);
}

fn v2sub(va, vb)
{
  return vec2(va.x - vb.x, va.y - vb.y);
}

fn v2scale(v, f)
{
  return vec2( v.x * f, v.y * f );
}

fn m4identity()
{
  return mat4(
    vec4(1.0, 0.0, 0.0, 0.0),
    vec4(0.0, 1.0, 0.0, 0.0),
    vec4(0.0, 0.0, 1.0, 0.0),
    vec4(0.0, 0.0, 0.0, 1.0),
  );
}

fn main()
{
  let pos = vec2(0.0, 1.0);
  let vel = vec2(1.0, 0.0);

  let r = v2add(pos, vel);
  println("Should be 2,1: ", v2add(r, vel));

  r.x = r.y = 0.1;

  println("Should be 10, 10: ", v2scale(r, 10.0));

  println("Identity matrix: ", m4identity());

  let u = vec2(0, 0);
  let a = vec2(0.1, 0.1);
  let t = 0.01;
  for (let i = 0 ; i < 1000; i++)
  {
    let v = u + a * t;
    println(v);
    t += 0.01;
  }

  u[0] = 69;
  u[1] = 32;
  println("Pootis: ", u);
}