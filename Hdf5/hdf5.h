// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2022, the Anboto author and contributors
#ifndef _hdf5_h_
#define _hdf5_h_

#include <plugin/Hdf5/src/hdf5.h>
#include <Eigen/Eigen.h>

namespace Upp {


class HidO {
public:
	HidO() {}
	HidO(hid_t loc_id, const char *name, hid_t lapl_id) {
		Open(loc_id, name, lapl_id);
	}
	hid_t Open(hid_t loc_id, const char *name, hid_t lapl_id) {
		id = H5Oopen(loc_id, name, lapl_id);
		return id;
	}
	~HidO() {
		if (id >= 0)
			H5Oclose(id);
	}
	operator hid_t() const {return id;};
	
private:
	hid_t id = -1;
};

class Hdf5File {
public:
	Hdf5File()				{}
	Hdf5File(String file)	{Load(file);}
	~Hdf5File()				{Close();}
	
	bool Load(String file);
	void Close();
	bool ChangeGroup(String group);
	Vector<String> ListGroup(bool groups, bool datasets);
	Vector<String> ListGroup()			{return ListGroup(true, true);}
	Vector<String> ListGroupGroups()	{return ListGroup(true, false);}
	Vector<String> ListGroupDatasets()	{return ListGroup(false, true);}
	void UpGroup();

	bool Exist(String name, bool isgroup);
	bool ExistDataset(String name)		{return Exist(name, false);}
	bool ExistGroup(String name)		{return Exist(name, true);}
	
	void GetType(String name, H5T_class_t &type, Vector<hsize_t> &dims);
	H5T_class_t GetType(String name) {
		H5T_class_t type;
		Vector<hsize_t> dummy;
		GetType(name, type, dummy);
		return type;
	}
	
	int GetInt(String name);
	double GetDouble(String name);
	String GetString(String name);

	void GetDouble(String name, Eigen::VectorXd &data);
	void GetDouble(String name, Eigen::MatrixXd &data);

private:
	hid_t file_id = Null;
	Vector<hid_t> group_ids;
	
	void GetData0(String name, HidO &obj_id, hid_t &datatype_id, hid_t &dspace, int &sz, Vector<hsize_t> &dims);
};

}
	
#endif

