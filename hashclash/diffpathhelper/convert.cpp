/**************************************************************************\
|
|    Copyright (C) 2009 Marc Stevens
|
|    This program is free software: you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation, either version 3 of the License, or
|    (at your option) any later version.
|
|    This program is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with this program.  If not, see <http://www.gnu.org/licenses/>.
|
\**************************************************************************/

#include <cmath>
#include <algorithm>
#include <map>

#include <hashutil5/saveload_gz.hpp>
#include <hashutil5/differentialpath.hpp>
#include <hashutil5/md5detail.hpp>
#include <hashutil5/booleanfunction.hpp>

#include <boost/lexical_cast.hpp>

#include "main.hpp"

void getline(std::istream& ifs)
{
	char c = 0;
	while (ifs && c != '\n' && c != '\r')
		ifs.get(c);
	while (ifs && (c == '\n' || c == '\r'))
		ifs.get(c);
	if (ifs)
		ifs.putback(c);
}

int pathfromtext(parameters_type& parameters)
{
	if (parameters.outfile1 == "") {
		cout << "No outputfile1 given!" << endl;
		return 2;
	}
	if (parameters.infile1 == "") {
		cout << "No inputfile1 given!" << endl;
		return 2;
	}
	parameters.show_mdiffs();
	differentialpath path;
	vector<differentialpath> vecpath;
	ifstream ifs(parameters.infile1.c_str());
	if (!ifs) {
		cerr << "Error: could not open " << parameters.infile1 << "!" << endl;
		return 1;
	}
	cout << "Parsing inputfile:";
	while (ifs) {
		cout << endl;
		char c;
		ifs >> c;
		cout << c << " ";
		if (!ifs) break;
		if (c != 'Q') {
			cout << "expected 'Q' here, going to next line";
			getline(ifs);
			continue;
		}
		int t;
		if (!(ifs >> t)) break;
		cout << t << " ";
		if (t < -3 || t > 64) {
			cout << "expected integer t with -3 <= t <= 64 here, going to next line";
			getline(ifs);
			continue;
		}
		while (ifs && c != '|') {
			ifs >> c;
			cout << c << " ";
		}
		if (c != '|') {
			cout << "expected '|' here, going to next line";
			getline(ifs);
			continue;
		}
		wordconditions wc;
		ifs >> wc.bytes[3] >> wc.bytes[2] >> wc.bytes[1] >> wc.bytes[0] >> c;
		cout << wc << " " << c << " ";
		if (!ifs) break;
		if (c != '|') {
			cout << "expected '|' here, going to next line";
			getline(ifs);
			continue;
		}
		path[t] = wc;
		getline(ifs);
	}
	cout << endl << "Parsed path:" << endl;
	show_path(path, parameters.m_diff);
	vecpath.push_back(path);
	cout << "Saving " << parameters.outfile1 << "..." << flush;
	try {
		save_gz(vecpath, binary_archive, parameters.outfile1);
		cout << "done." << endl;
	} catch (...) {
		cout << "failed." << endl;
		return 1;
	}
	return 0;
}

int split(parameters_type& parameters)
{
	vector<differentialpath> vecpath, splitvec;
	cout << "Loading " << parameters.infile1 << "..." << flush;
	try {
		load_gz(vecpath, binary_archive, parameters.infile1);
		cout << "done (loaded " << vecpath.size() << " paths)." << endl;
	} catch (...) {
		cout << "failed." << endl;
		return 1;
	}
	double step = double(vecpath.size()) / double(parameters.split);
	uint32 start = 0;
	uint32 end = vecpath.size();
	for (unsigned i = 0; i < parameters.split; ++i)
	{
		uint32 iend = uint32(double(step*double(i+1)));
		if (i+1 == parameters.split || iend > end)
			iend = end;
		splitvec.clear();
		splitvec.reserve(iend-start);
		for (unsigned j = start; j < iend; ++j)
			splitvec.push_back(vecpath[j]);
		start = iend;
		string filename = parameters.infile1 + "_" 
					+ boost::lexical_cast<string>(i) + "of"
					+ boost::lexical_cast<string>(parameters.split) + ".bin.gz";
		cout << "Saving " << splitvec.size() << " paths to " << filename << "..." << flush;
		try {
			save_gz(splitvec, binary_archive, filename);
			cout << "done." << endl;
		} catch (...) {
			cout << "failed." << endl;
			return 1;
		}
	}
	return 0;
}

int join(parameters_type& parameters)
{
	if (parameters.outfile1 == "") {
		cout << "No outputfile1 given!" << endl;
		return 2;
	}
	vector<differentialpath> vecpath, joinvec;
	for (unsigned i = 0; i < parameters.files.size(); ++i)
	{
		joinvec.clear();
		cout << "Loading " << parameters.files[i] << "..." << flush;
		try {
			load_gz(joinvec, binary_archive, parameters.files[i]);
			cout << "done (loaded " << joinvec.size() << " paths)." << endl;
			for (unsigned j = 0; j < joinvec.size(); ++j)
				vecpath.push_back(joinvec[j]);
		} catch (...) {
			cout << "failed." << endl;
		}
	}
	cout << "Saving " << vecpath.size() << " paths to " << parameters.outfile1 << "..." << flush;
	try {
		save_gz(vecpath, binary_archive, parameters.outfile1);
		cout << "done." << endl;
	} catch (...) {
		cout << "failed." << endl;
		return 1;
	}
	return 0;
}

int convert(parameters_type& parameters)
{	
	differentialpath path;	
	vector<differentialpath> vecpath;
	bool failed = true;
	if (failed) {
		cout << "Trying to read vector of differential paths in binary..." << flush;
		try {
			load_gz(vecpath, binary_archive, parameters.infile1);
			if (vecpath.size() > 0) {
				failed = false;
				cout << "success." << endl;
				cout << "Trying to save vector of differential paths in text..." << flush;
				save_gz(vecpath, text_archive, parameters.infile1 + ".txt.gz");
				cout << "success." << endl;
			}
		} catch (...) {
			cout << "failed." << endl;
		}
	}
	if (failed) {
		cout << "Trying to read vector of differential paths in text..." << flush;
		try {
			load_gz(vecpath, text_archive, parameters.infile1);
			if (vecpath.size() > 0) {
				failed = false;
				cout << "success." << endl;
				cout << "Trying to save vector of differential paths in binary..." << flush;
				save_gz(vecpath, binary_archive, parameters.infile1 + ".bin.gz");
				cout << "success." << endl;
			}
		} catch (...) {
			cout << "failed." << endl;
		}
	}
	if (failed) {
		cout << "Trying to read differential path in binary..." << flush;
		try {
			load_gz(path, binary_archive, parameters.infile1);
			failed = false;
			cout << "success." << endl;
			cout << "Trying to save differential path in text..." << flush;
			save_gz(path, text_archive, parameters.infile1 + ".txt.gz");
			cout << "success." << endl;
		} catch (...) {
			cout << "failed." << endl;
		}
	}
	if (failed) {
		cout << "Trying to read differential path in text..." << flush;
		try {
			load_gz(path, text_archive, parameters.infile1);
			failed = false;
			cout << "success." << endl;
			cout << "Trying to save differential path in binary..." << flush;
			save_gz(path, binary_archive, parameters.infile1 + ".bin.gz");
			cout << "success." << endl;
		} catch (...) {
			cout << "failed." << endl;
		}
	}
	if (failed)
		return 1;
	return 0;
}

void reconstructpath(const uint32 ihv1[], const uint32 ihv2[]
					, const uint32 msg1[], const uint32 msg2[]
					, ofstream& o)
{
	const unsigned offset = 3;
	uint32 Q1[69], Q2[69];
	Q1[0] = ihv1[0]; Q1[1] = ihv1[3]; Q1[2] = ihv1[2]; Q1[3] = ihv1[1];
	Q2[0] = ihv2[0]; Q2[1] = ihv2[3]; Q2[2] = ihv2[2]; Q2[3] = ihv2[1];
	uint32 mdiff[16];
	for (unsigned k = 0; k < 16; ++k)
		mdiff[k] = msg2[k] - msg1[k];

	differentialpath path;
	for (int i = 0; i < 4; ++i)
		for (unsigned k = 0; k < 32; ++k)
		{
			if ((Q1[i]>>k)&1) {
				if ((Q2[i]>>k)&1)
					path.setbitcondition(i-3,k,bc_one);
				else
					path.setbitcondition(i-3,k,bc_minus);
			} else {
				if ((Q2[i]>>k)&1)
					path.setbitcondition(i-3,k,bc_plus);
				else
					path.setbitcondition(i-3,k,bc_zero);
			}
		}
	for (int t = 0; t < 16; ++t) {
		uint32 F1 = md5_ff(Q1[offset+t], Q1[offset+t-1], Q1[offset+t-2]);
		uint32 F2 = md5_ff(Q2[offset+t], Q2[offset+t-1], Q2[offset+t-2]);
		for (unsigned k = 0; k < 32; ++k)
		{
			int d = int((F2>>k)&1) - int((F1>>k)&1);			
			bf_conditions bc = MD5_F_data.forwardconditions(path(t,k), path(t-1,k), path(t-2,k), diffbitcondition(d));
			path[t].set(k, bc.first);
			path[t-1].set(k, bc.second);
			path[t-2].set(k, bc.third);
		}
		uint32 T1 = F1 + Q1[offset+t-3] + msg1[md5_wt[t]] + md5_ac[t];
		uint32 T2 = F2 + Q2[offset+t-3] + msg2[md5_wt[t]] + md5_ac[t];
		uint32 R1 = rotate_left(T1, md5_rc[t]);
		uint32 R2 = rotate_left(T2, md5_rc[t]);
		Q1[offset+t+1] = Q1[offset+t] + R1;
		Q2[offset+t+1] = Q2[offset+t] + R2;
		path[t+1].set(sdr(Q1[offset+t+1], Q2[offset+t+1]));
	}
	for (int t = 16; t < 32; ++t) {
		uint32 F1 = md5_gg(Q1[offset+t], Q1[offset+t-1], Q1[offset+t-2]);
		uint32 F2 = md5_gg(Q2[offset+t], Q2[offset+t-1], Q2[offset+t-2]);
		for (unsigned k = 0; k < 32; ++k)
		{
			int d = int((F2>>k)&1) - int((F1>>k)&1);			
			bf_conditions bc = MD5_G_data.forwardconditions(path(t,k), path(t-1,k), path(t-2,k), diffbitcondition(d));
			path[t].set(k, bc.first);
			path[t-1].set(k, bc.second);
			path[t-2].set(k, bc.third);
		}
		uint32 T1 = F1 + Q1[offset+t-3] + msg1[md5_wt[t]] + md5_ac[t];
		uint32 T2 = F2 + Q2[offset+t-3] + msg2[md5_wt[t]] + md5_ac[t];
		uint32 R1 = rotate_left(T1, md5_rc[t]);
		uint32 R2 = rotate_left(T2, md5_rc[t]);
		Q1[offset+t+1] = Q1[offset+t] + R1;
		Q2[offset+t+1] = Q2[offset+t] + R2;
		path[t+1].set(sdr(Q1[offset+t+1], Q2[offset+t+1]));
	}
	for (int t = 32; t < 48; ++t) {
		uint32 F1 = md5_hh(Q1[offset+t], Q1[offset+t-1], Q1[offset+t-2]);
		uint32 F2 = md5_hh(Q2[offset+t], Q2[offset+t-1], Q2[offset+t-2]);
		for (unsigned k = 0; k < 32; ++k)
		{
			int d = int((F2>>k)&1) - int((F1>>k)&1);			
			bf_conditions bc = MD5_H_data.forwardconditions(path(t,k), path(t-1,k), path(t-2,k), diffbitcondition(d));
			path[t].set(k, bc.first);
			path[t-1].set(k, bc.second);
			path[t-2].set(k, bc.third);
		}
		uint32 T1 = F1 + Q1[offset+t-3] + msg1[md5_wt[t]] + md5_ac[t];
		uint32 T2 = F2 + Q2[offset+t-3] + msg2[md5_wt[t]] + md5_ac[t];
		uint32 R1 = rotate_left(T1, md5_rc[t]);
		uint32 R2 = rotate_left(T2, md5_rc[t]);
		Q1[offset+t+1] = Q1[offset+t] + R1;
		Q2[offset+t+1] = Q2[offset+t] + R2;
		path[t+1].set(sdr(Q1[offset+t+1], Q2[offset+t+1]));
	}
	for (int t = 48; t < 64; ++t) {
		uint32 F1 = md5_ii(Q1[offset+t], Q1[offset+t-1], Q1[offset+t-2]);
		uint32 F2 = md5_ii(Q2[offset+t], Q2[offset+t-1], Q2[offset+t-2]);
		for (unsigned k = 0; k < 32; ++k)
		{
			int d = int((F2>>k)&1) - int((F1>>k)&1);			
			bf_conditions bc = MD5_I_data.forwardconditions(path(t,k), path(t-1,k), path(t-2,k), diffbitcondition(d));
			path[t].set(k, bc.first);
			path[t-1].set(k, bc.second);
			path[t-2].set(k, bc.third);
		}
		uint32 T1 = F1 + Q1[offset+t-3] + msg1[md5_wt[t]] + md5_ac[t];
		uint32 T2 = F2 + Q2[offset+t-3] + msg2[md5_wt[t]] + md5_ac[t];
		uint32 R1 = rotate_left(T1, md5_rc[t]);
		uint32 R2 = rotate_left(T2, md5_rc[t]);
		Q1[offset+t+1] = Q1[offset+t] + R1;
		Q2[offset+t+1] = Q2[offset+t] + R2;
		path[t+1].set(sdr(Q1[offset+t+1], Q2[offset+t+1]));
	}
	for (int t = 63; t >= 48; --t)
		for (unsigned k = 0; k < 32; ++k)
		{
			bf_outcome oc = MD5_I_data.outcome(path(t,k), path(t-1,k), path(t-2,k));
			bf_conditions bc = MD5_I_data.backwardconditions(path(t,k), path(t-1,k), path(t-2,k), oc[0]);
			path[t].set(k, bc.first);
			path[t-1].set(k, bc.second);
			path[t-2].set(k, bc.third);
		}
	for (int t = 47; t >= 32; --t)
		for (unsigned k = 0; k < 32; ++k)
		{
			bf_outcome oc = MD5_H_data.outcome(path(t,k), path(t-1,k), path(t-2,k));
			bf_conditions bc = MD5_H_data.backwardconditions(path(t,k), path(t-1,k), path(t-2,k), oc[0]);
			path[t].set(k, bc.first);
			path[t-1].set(k, bc.second);
			path[t-2].set(k, bc.third);
		}
	for (int t = 31; t >= 16; --t)
		for (unsigned k = 0; k < 32; ++k)
		{
			bf_outcome oc = MD5_G_data.outcome(path(t,k), path(t-1,k), path(t-2,k));
			bf_conditions bc = MD5_G_data.backwardconditions(path(t,k), path(t-1,k), path(t-2,k), oc[0]);
			path[t].set(k, bc.first);
			path[t-1].set(k, bc.second);
			path[t-2].set(k, bc.third);
		}
	for (int t = 15; t >= 0; --t)
		for (unsigned k = 0; k < 32; ++k)
		{
			bf_outcome oc = MD5_F_data.outcome(path(t,k), path(t-1,k), path(t-2,k));
			bf_conditions bc = MD5_F_data.backwardconditions(path(t,k), path(t-1,k), path(t-2,k), oc[0]);
			path[t].set(k, bc.first);
			path[t-1].set(k, bc.second);
			path[t-2].set(k, bc.third);
		}
	enhancepath(path, mdiff);
	show_path(path, mdiff, o);
	o << endl << endl;
}

int pathfromcollision(parameters_type& parameters)
{
	if (parameters.infile1 == "") {
		cout << "No inputfile1 given!" << endl;
		return 2;
	}
	if (parameters.infile2 == "") {
		cout << "No inputfile2 given!" << endl;
		return 2;
	}
	if (parameters.outfile1 == "") {
		cout << "No outputfile given!" << endl;
		return 2;
	}	

	ifstream if1(parameters.infile1.c_str(), ios::binary);
	ifstream if2(parameters.infile2.c_str(), ios::binary);
	ofstream of(parameters.outfile1.c_str());

	uint32 ihv1[4];
	uint32 ihv2[4];
	uint32 msg1[16];
	uint32 msg2[16];

	for (unsigned k = 0; k < 4; ++k)
		ihv1[k] = ihv2[k] = md5_iv[k];

	unsigned bytes1, bytes2;
	while (true) {
		bytes1 = load_block(if1, msg1);
		bytes2 = load_block(if2, msg2);
		if (bytes1 == 0 || bytes2 == 0)
			break;
		unsigned w = 0;
		for (unsigned k = 0; k < 16; ++k)
			w += hwnaf(msg2[k] - msg1[k]);
		if (w > 0 && w <= 8) {
			reconstructpath(ihv1, ihv2, msg1, msg2, of);
		}
		md5compress(ihv1, msg1);
		md5compress(ihv2, msg2);
	}
	return 0;
}
