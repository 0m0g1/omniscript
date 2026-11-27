// docs/examples/bench/langs/bench.go
// Go reference for the OmniScript benchmark suite. Same algorithms, sizes and checks as bench.c.
// Build: go build -o bench_go.exe bench.go        Run: bench_go fib|sieve|nbody|spectral|fannkuch|trees [n]
package main

import (
	"fmt"
	"math"
	"os"
	"strconv"
)

func fib(n int) int64 {
	if n < 2 {
		return int64(n)
	}
	return fib(n-1) + fib(n-2)
}

func sieve(n int) int64 {
	s := make([]byte, n+1)
	var count int64
	for i := 2; i <= n; i++ {
		if s[i] == 0 {
			count++
			for j := i * i; j <= n; j += i {
				s[j] = 1
			}
		}
	}
	return count
}

const (
	PI        = 3.141592653589793
	SolarMass = 4 * PI * PI
	Days      = 365.24
)

var x, y, z, vx, vy, vz, m [5]float64

func nbInit() {
	X := [5]float64{0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01}
	Y := [5]float64{0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01}
	Z := [5]float64{0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01}
	VX := [5]float64{0, 1.66007664274403694e-03, -2.76742510726862411e-03, 2.96460137564761618e-03, 2.68067772490389322e-03}
	VY := [5]float64{0, 7.69901118419740425e-03, 4.99852801234917238e-03, 2.37847173959480950e-03, 1.62824170038242295e-03}
	VZ := [5]float64{0, -6.90460016972063023e-05, 2.30417297573763929e-05, -2.96589568540237556e-05, -9.51592254519715870e-05}
	M := [5]float64{1, 9.54791938424326609e-04, 2.85885980666130812e-04, 4.36624404335156298e-05, 5.15138902046611451e-05}
	for i := 0; i < 5; i++ {
		x[i], y[i], z[i] = X[i], Y[i], Z[i]
		vx[i], vy[i], vz[i] = VX[i]*Days, VY[i]*Days, VZ[i]*Days
		m[i] = M[i] * SolarMass
	}
	var px, py, pz float64
	for i := 0; i < 5; i++ {
		px += vx[i] * m[i]
		py += vy[i] * m[i]
		pz += vz[i] * m[i]
	}
	vx[0], vy[0], vz[0] = -px/SolarMass, -py/SolarMass, -pz/SolarMass
}

func nbEnergy() float64 {
	e := 0.0
	for i := 0; i < 5; i++ {
		e += 0.5 * m[i] * (vx[i]*vx[i] + vy[i]*vy[i] + vz[i]*vz[i])
		for j := i + 1; j < 5; j++ {
			dx, dy, dz := x[i]-x[j], y[i]-y[j], z[i]-z[j]
			e -= (m[i] * m[j]) / math.Sqrt(dx*dx+dy*dy+dz*dz)
		}
	}
	return e
}

func nbAdvance(dt float64) {
	for i := 0; i < 5; i++ {
		for j := i + 1; j < 5; j++ {
			dx, dy, dz := x[i]-x[j], y[i]-y[j], z[i]-z[j]
			d2 := dx*dx + dy*dy + dz*dz
			mag := dt / (d2 * math.Sqrt(d2))
			vx[i] -= dx * m[j] * mag
			vy[i] -= dy * m[j] * mag
			vz[i] -= dz * m[j] * mag
			vx[j] += dx * m[i] * mag
			vy[j] += dy * m[i] * mag
			vz[j] += dz * m[i] * mag
		}
	}
	for i := 0; i < 5; i++ {
		x[i] += dt * vx[i]
		y[i] += dt * vy[i]
		z[i] += dt * vz[i]
	}
}

func spA(i, j int) float64 { return 1.0 / float64((i+j)*(i+j+1)/2+i+1) }

func spMulAv(n int, v, av []float64) {
	for i := 0; i < n; i++ {
		s := 0.0
		for j := 0; j < n; j++ {
			s += spA(i, j) * v[j]
		}
		av[i] = s
	}
}

func spMulAtv(n int, v, atv []float64) {
	for i := 0; i < n; i++ {
		s := 0.0
		for j := 0; j < n; j++ {
			s += spA(j, i) * v[j]
		}
		atv[i] = s
	}
}

func spectral(n int) float64 {
	u, v, t := make([]float64, n), make([]float64, n), make([]float64, n)
	for i := range u {
		u[i] = 1.0
	}
	for k := 0; k < 10; k++ {
		spMulAv(n, u, t)
		spMulAtv(n, t, v)
		spMulAv(n, v, t)
		spMulAtv(n, t, u)
	}
	vbv, vv := 0.0, 0.0
	for i := 0; i < n; i++ {
		vbv += u[i] * v[i]
		vv += v[i] * v[i]
	}
	return math.Sqrt(vbv / vv)
}

func fannkuch(n int) (int64, int) {
	var p, q, s [32]int
	sign := 1
	var sum int64
	maxf := 0
	for i := 0; i < n; i++ {
		p[i], q[i], s[i] = i, i, i
	}
	for {
		q0 := p[0]
		if q0 != 0 {
			for i := 1; i < n; i++ {
				q[i] = p[i]
			}
			flips := 1
			for {
				qq := q[q0]
				if qq == 0 {
					sum += int64(sign * flips)
					if flips > maxf {
						maxf = flips
					}
					break
				}
				q[q0] = q0
				if q0 >= 3 {
					i, j := 1, q0-1
					for {
						q[i], q[j] = q[j], q[i]
						i++
						j--
						if !(i < j) {
							break
						}
					}
				}
				q0 = qq
				flips++
			}
		}
		if sign == 1 {
			p[1], p[0] = p[0], p[1]
			sign = -1
		} else {
			p[1], p[2] = p[2], p[1]
			sign = 1
			for i := 2; i < n; i++ {
				sx := s[i]
				if sx != 0 {
					s[i] = sx - 1
					break
				}
				if i == n-1 {
					return sum, maxf
				}
				s[i] = i
				t := p[0]
				for j := 0; j <= i; j++ {
					p[j] = p[j+1]
				}
				p[i+1] = t
			}
		}
	}
}

type Node struct{ l, r *Node }

func mk(d int) *Node {
	if d > 0 {
		return &Node{mk(d - 1), mk(d - 1)}
	}
	return &Node{}
}

func chk(n *Node) int64 {
	if n.l != nil {
		return 1 + chk(n.l) + chk(n.r)
	}
	return 1
}

func trees(maxd int) int64 {
	var total int64
	for d := 4; d <= maxd; d += 4 {
		iters := int64(1) << uint(maxd-d+4)
		for i := int64(0); i < iters; i++ {
			total += chk(mk(d))
		}
	}
	return total
}

func main() {
	b := ""
	if len(os.Args) > 1 {
		b = os.Args[1]
	}
	n := -1
	if len(os.Args) > 2 {
		n, _ = strconv.Atoi(os.Args[2])
	}
	pick := func(def int) int {
		if n < 0 {
			return def
		}
		return n
	}
	switch b {
	case "fib":
		fmt.Println(fib(pick(40)))
	case "sieve":
		fmt.Println(sieve(pick(100000000)))
	case "nbody":
		nbInit()
		fmt.Printf("%.9f\n", nbEnergy())
		for i, s := 0, pick(50000000); i < s; i++ {
			nbAdvance(0.01)
		}
		fmt.Printf("%.9f\n", nbEnergy())
	case "spectral":
		fmt.Printf("%.9f\n", spectral(pick(5500)))
	case "fannkuch":
		nn := pick(11)
		c, f := fannkuch(nn)
		fmt.Printf("%d\nPfannkuchen(%d) = %d\n", c, nn, f)
	case "trees":
		fmt.Println(trees(pick(20)))
	default:
		fmt.Fprintln(os.Stderr, "usage: bench_go fib|sieve|nbody|spectral|fannkuch|trees [n]")
		os.Exit(1)
	}
}
