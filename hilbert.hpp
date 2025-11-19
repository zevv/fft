
// https://yehar.com/blog/?p=368

#pragma once

#include <complex>


class Hilbert {
private:

	class H {
	private:
		double a2, x1{}, x2{}, y1{}, y2{};
	public:
		H(double a = 0.0) : a2(a * a) {}
		double run(double x0) {
			double y0 = a2 * (x0 + y2) - x2;
			x2 = x1; x1 = x0; y2 = y1; y1 = y0;
			return y0;
		}

	};


	H h[2][4];
    double vI_prev{};

public:
    Hilbert() {
        h[0][0] = H(0.6923878000000);
        h[0][1] = H(0.9360654322959);
        h[0][2] = H(0.9882295226860);
        h[0][3] = H(0.9987488452737);
        h[1][0] = H(0.4021921162426);
        h[1][1] = H(0.8561710882420);
        h[1][2] = H(0.9722909545651);
        h[1][3] = H(0.9952884791278);
    }

	std::complex<double> run(double input) {
        double vI = input, vQ = input;
        for (auto& section : h[0]) vI = section.run(vI);
        for (auto& section : h[1]) vQ = section.run(vQ);
		auto result = std::complex<double>(vI_prev, vQ);
        vI_prev = vI;
		return result;
    }

};


