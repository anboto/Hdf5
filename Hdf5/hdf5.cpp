// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2023, the Anboto author and contributors
#include <Core/Core.h>
#include <Eigen/Eigen.h>

#include "hdf5.h"

namespace Upp {


void Hdf5File::Close() {
	for (int i = group_ids.size()-1; i >= 0; --i)
		H5Gclose(group_ids[i]);
	
	dts_id.Close();
	
	if (file_id >= 0) {
		ssize_t num_tot = H5Fget_obj_count(file_id, H5F_OBJ_ALL);
		if (num_tot != 1)
			throw Exc("Unclosed objects");
		H5Fclose(file_id);
	}
}

bool Hdf5File::Open(String file, unsigned mode) {
	Close();
	
	if (!FileExists(file))
		return false;
	
    file_id = H5Fopen(file, mode, H5P_DEFAULT);
    if (file_id < 0) 
        return false;
    
    hid_t group_id = H5Gopen2(file_id, "/", H5P_DEFAULT);
    if (group_id < 0) 
        throw Exc("Unable to open root group");
	group_ids << group_id;
	
	return true;
}

bool Hdf5File::Create(String file) {
	Close();
	
    file_id = H5Fcreate(file, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) 
        return false;	

    hid_t group_id = H5Gopen2(file_id, "/", H5P_DEFAULT);
    if (group_id < 0) 
        throw Exc("Unable to open root group");
	group_ids << group_id;
	
	return true;
}

bool Hdf5File::CreateGroup(String group, bool change) {
	hid_t group_id = Last(group_ids);

    hid_t id = H5Gcreate2(group_id, group, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (id < 0) 
        return false;

	if (change) {
		hid_t ngroup_id = H5Oopen(group_id, ~group, H5P_DEFAULT);
		group_ids << ngroup_id;
	}

	H5Gclose(id);

	return true;
}

bool Hdf5File::ChangeGroup(String sgroup) {
	hid_t group_id = Last(group_ids);
	
    H5G_info_t group_info;
    H5Gget_info(group_id, &group_info);
	
    for (hsize_t i = 0; i < group_info.nlinks; i++) {
        char obj_name[256];
        H5Lget_name_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, obj_name, sizeof(obj_name), H5P_DEFAULT);
		String sname(obj_name);

		if (sname == sgroup && H5Lexists(group_id, obj_name, H5P_DEFAULT) >= 0) {
            hid_t ngroup_id = H5Oopen(group_id, obj_name, H5P_DEFAULT);
            H5O_info2_t oinfo;
            if (H5Oget_info(ngroup_id, &oinfo, H5O_INFO_BASIC) >= 0) {
            	if (oinfo.type == H5O_TYPE_GROUP) {
	            	group_ids << ngroup_id;
	            	return true;
            	}
            	H5Oclose(ngroup_id);
            }
		}
    }
    return false;
}

Vector<String> Hdf5File::ListGroup(bool groups, bool datasets) {
	Vector<String> ret;
	
	hid_t group_id = Last(group_ids);
	
    H5G_info_t group_info;
    H5Gget_info(group_id, &group_info);
	
    for (hsize_t i = 0; i < group_info.nlinks; i++) {
        char obj_name[256];
        H5Lget_name_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, obj_name, sizeof(obj_name), H5P_DEFAULT);
        if (groups && datasets)
			ret << String(obj_name);
        else {
        	if (H5Lexists(group_id, obj_name, H5P_DEFAULT) >= 0) {
	            hid_t obj_id = H5Oopen(group_id, obj_name, H5P_DEFAULT);
	            H5O_info2_t oinfo;
	            if (H5Oget_info(obj_id, &oinfo, H5O_INFO_BASIC) >= 0) {
	            	if (groups && oinfo.type == H5O_TYPE_GROUP || datasets && oinfo.type == H5O_TYPE_DATASET) 
	            		ret << String(obj_name);
	            }
	            H5Oclose(obj_id);
        	}
        }
    }
	return ret;
}

bool Hdf5File::Exist(String name, bool isgroup) {
	hid_t group_id = Last(group_ids);
	
    H5G_info_t group_info;
    H5Gget_info(group_id, &group_info);
	
    for (hsize_t i = 0; i < group_info.nlinks; i++) {
        char obj_name[256];
        H5Lget_name_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, obj_name, sizeof(obj_name), H5P_DEFAULT);
		String ename(obj_name);
		if (ename == name) {
	        if (H5Lexists(group_id, obj_name, H5P_DEFAULT) >= 0) {
		    	HidO obj_id = H5Oopen(group_id, obj_name, H5P_DEFAULT);
		        H5O_info2_t oinfo;
		        if (H5Oget_info(obj_id, &oinfo, H5O_INFO_BASIC) >= 0) {
		           	if (isgroup) {
		           		if (oinfo.type == H5O_TYPE_GROUP)
		           			return true;
		           	} else {
		           		if (oinfo.type == H5O_TYPE_DATASET) 
		           			return true;
		           	}
	            }
	        }
		}
    }
	return false;
}

bool Hdf5File::Delete(String name) {
	hid_t group_id = Last(group_ids);
	
	if (H5Ldelete(group_id, name, H5P_DEFAULT) < 0) 
        return false;
    return true;
}

void Hdf5File::UpGroup() {
	H5Gclose(Last(group_ids));	
	group_ids.Remove(group_ids.size()-1);
}

void Hdf5File::GetData0(String name, HidO &obj_id, hid_t &datatype_id, hid_t &dspace, int &sz, Vector<hsize_t> &dims) {
	hid_t group_id = Last(group_ids);
	
	if (H5Lexists(group_id, ~name, H5P_DEFAULT) < 0) 
		throw Exc("Dataset not found");
		
  	obj_id = H5Oopen(group_id, ~name, H5P_DEFAULT);
    H5O_info2_t oinfo;
    if (H5Oget_info(obj_id, &oinfo, H5O_INFO_BASIC) < 0) 
        throw Exc("Dataset info not found");
        
    if (oinfo.type != H5O_TYPE_DATASET)
        throw Exc("It is not a dataset");
    
    datatype_id = H5Dget_type(obj_id);
	if (datatype_id < 0)
		throw Exc("Dataset type is unknown");
		
	dspace = H5Dget_space(obj_id);
	const int ndims = H5Sget_simple_extent_ndims(dspace);
	
	dims.SetCount(ndims);
	H5Sget_simple_extent_dims(dspace, dims.begin(), NULL);
	   
    sz = 1;
    for (int id = 0; id < ndims; ++id) 
        sz *= int(dims[id]);
}

void Hdf5File::GetType(String name, H5T_class_t &type, Vector<hsize_t> &dims) {
	int sz;
	HidO obj_id;
	hid_t datatype_id, dspace;
	GetData0(name, obj_id, datatype_id, dspace, sz, dims);
	type = H5Tget_class(datatype_id);
}

int Hdf5File::GetInt(String name) {
	int sz;
	HidO obj_id;
	hid_t datatype_id, dspace;
	Vector<hsize_t> dims;
	GetData0(name, obj_id, datatype_id, dspace, sz, dims);
	
	H5T_class_t clss = H5Tget_class(datatype_id);
	if (clss != H5T_INTEGER)
		throw Exc("Dataset is not integer");
	
	if (sz != 1) 
		throw Exc("Size is not 1");

	int i;
    if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, &i) < 0) 
        throw Exc("Impossible to read data");
    return i;
}

double Hdf5File::GetDouble(String name) {
	int sz;
	HidO obj_id;
	hid_t datatype_id, dspace;
	Vector<hsize_t> dims;
	GetData0(name, obj_id, datatype_id, dspace, sz, dims);

	H5T_class_t clss = H5Tget_class(datatype_id);
	if (clss != H5T_FLOAT)
		throw Exc("Dataset is not double");
	
	if (sz != 1) 
		throw Exc("Size is not 1");
	
	double d;
    if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, &d) < 0) 
        throw Exc("Impossible to read data");
    return d;
}

String Hdf5File::GetString(String name) {
	int sz;
	HidO obj_id;
	hid_t datatype_id, dspace;
	Vector<hsize_t> dims;
	GetData0(name, obj_id, datatype_id, dspace, sz, dims);
	
	H5T_class_t clss = H5Tget_class(datatype_id);
	if (clss != H5T_STRING)
		throw Exc("Dataset is not string");
	
	if (sz != 1) 
		throw Exc("Size is not 1");
	
	hsize_t len = H5Dget_storage_size(obj_id);
    for (int id = 0; id < dims.size(); ++id) 
		len /= dims[id];
    
	H5S_class_t space_class = H5Sget_simple_extent_type(dspace);
	
    if (space_class == H5S_SCALAR) {
        StringBuffer bstr((int)len);
		if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, ~bstr) >= 0) 
			return String(bstr);
		throw Exc("Problem reading scalar string");
    } else {
        hsize_t size = H5Sget_simple_extent_npoints(dspace);
    	Buffer<char *> bstr(size);
		if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, ~bstr) >= 0) 
			return String(bstr[0]);
		throw Exc("Problem reading string");
    }
	return Null;
}

void Hdf5File::GetDouble(String name, Eigen::VectorXd &data) {
	int sz;
	HidO obj_id;
	hid_t datatype_id, dspace;
	Vector<hsize_t> dims;
	GetData0(name, obj_id, datatype_id, dspace, sz, dims);
	
	if (!(dims.size() == 1) && (dims.size() == 2 && dims[0] != 1 && dims[1] != 1))
		throw Exc("Dimension different than one");
	
	H5T_class_t clss = H5Tget_class(datatype_id);
	if (clss != H5T_FLOAT)
		throw Exc("Dataset is not double");
	
	data.resize(dims[0]);
	if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data()) < 0) 
		throw Exc("Impossible to read data");
}

void Hdf5File::GetDouble(String name, Vector<double> &data) {
	int sz;
	HidO obj_id;
	hid_t datatype_id, dspace;
	Vector<hsize_t> dims;
	GetData0(name, obj_id, datatype_id, dspace, sz, dims);
	
	if (!(dims.size() == 1) && (dims.size() == 2 && dims[0] != 1 && dims[1] != 1))
		throw Exc("Dimension different than one");
	
	H5T_class_t clss = H5Tget_class(datatype_id);
	if (clss != H5T_FLOAT)
		throw Exc("Dataset is not double");
	
	data.SetCount(dims[0]);
	if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.begin()) < 0) 
		throw Exc("Impossible to read data");
}

void Hdf5File::GetDouble(String name, Eigen::MatrixXd &data) {
	int sz;
	HidO obj_id;
	hid_t datatype_id, dspace;
	Vector<hsize_t> dims;
	GetData0(name, obj_id, datatype_id, dspace, sz, dims);
	
	if (dims.size() != 2)
		throw Exc("Dimension different than two");
	
	H5T_class_t clss = H5Tget_class(datatype_id);
	if (clss != H5T_FLOAT)
		throw Exc("Dataset is not double");
	
	Vector<double> d(sz);
	if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, d.begin()) < 0) 
		throw Exc("Impossible to read data");
	
	CopyRowMajor(d, dims[0], dims[1], data);
}

void Hdf5File::GetDouble(String name, Buffer<double> &d, MultiDimMatrixIndex &indx) {
	int sz;
	HidO obj_id;
	hid_t datatype_id, dspace;
	Vector<hsize_t> dims;
	GetData0(name, obj_id, datatype_id, dspace, sz, dims);
	
	indx.SetNumAxis(dims.size());
	for (int i = 0; i < dims.size(); ++i)
		indx.SetAxisDim(i, dims[i]);
	
	H5T_class_t clss = H5Tget_class(datatype_id);
	if (clss != H5T_FLOAT)
		throw Exc("Dataset is not double");
	
	d.Alloc(sz);
	if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, d.begin()) < 0) 
		throw Exc("Impossible to read data");
}
	
void Hdf5File::SetAttributes0(hid_t dset_id, String attribute, String val) {
    hid_t attr_id_desc = H5Screate(H5S_SCALAR);
    hid_t attr_type_desc = H5Tcopy(H5T_C_S1);
    H5Tset_size(attr_type_desc, H5T_VARIABLE);
    hid_t attr_desc = H5Acreate2(dset_id, ~attribute, attr_type_desc, attr_id_desc, H5P_DEFAULT, H5P_DEFAULT);
    const char *p = val.begin();
    if (H5Awrite(attr_desc, attr_type_desc, &p) < 0)
        throw Exc("Impossible to write attribute");
    H5Aclose(attr_desc);
    H5Tclose(attr_type_desc);
    H5Sclose(attr_id_desc);
}

Hdf5File &Hdf5File::SetDescription(String description) {
	if (!IsNull(description))
		SetAttributes0(dts_id, "description", description);
	return *this;
}

Hdf5File &Hdf5File::SetUnits(String units) {
	if (!IsNull(units))
		SetAttributes0(dts_id, "units", units);
	return *this;
}
    
Hdf5File &Hdf5File::Set(String name, int d) {
	if (ExistDataset(name))
		Delete(name);
	
	hsize_t dims[1] = {1};
    HidS dataspace_id = H5Screate_simple(1, dims, NULL);
    if (dataspace_id < 0) 
        throw Exc("Error creating dataspace");
    
    if ((dts_id = H5Dcreate2(Last(group_ids), name, H5T_NATIVE_INT, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        throw Exc("Error creating dataset");

    if (H5Dwrite(dts_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &d) < 0) 
        throw Exc("Error writing data to dataset");
    
    return *this;
}

Hdf5File &Hdf5File::Set(String name, double d) {
	if (ExistDataset(name))
		Delete(name);
		
	hsize_t dims[1] = {1};
    HidS dataspace_id = H5Screate_simple(1, dims, NULL);
    if (dataspace_id < 0) 
        throw Exc("Error creating dataspace");
    
    if ((dts_id = H5Dcreate2(Last(group_ids), name, H5T_NATIVE_DOUBLE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        throw Exc("Error creating dataset");

    if (H5Dwrite(dts_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &d) < 0) 
        throw Exc("Error writing data to dataset");
    
    return *this;
}

Hdf5File &Hdf5File::Set(String name, String d) {
	if (ExistDataset(name))
		Delete(name);
		
    hid_t datatype_id = H5Tcopy(H5T_C_S1);
    H5Tset_size(datatype_id, H5T_VARIABLE);
    
	hsize_t dims[1] = {1};
    HidS dataspace_id = H5Screate_simple(1, dims, NULL);
    if (dataspace_id < 0) 
        throw Exc("Error creating dataspace");
    
    if ((dts_id = H5Dcreate2(Last(group_ids), name, datatype_id, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        throw Exc("Error creating dataset");

	const char *p = d.begin();
    if (H5Dwrite(dts_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, &p) < 0) 
        throw Exc("Error writing data to dataset");
    
    return *this;
}

Hdf5File &Hdf5File::Set(String name, const Eigen::VectorXd &d) {
	if (ExistDataset(name))
		Delete(name);
		
	hsize_t dims[1];
	dims[0] = d.size();
    HidS dataspace_id = H5Screate_simple(1, dims, NULL);
    if (dataspace_id < 0) 
        throw Exc("Error creating dataspace");
    
    if ((dts_id = H5Dcreate2(Last(group_ids), name, H5T_NATIVE_DOUBLE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        throw Exc("Error creating dataset");

    if (H5Dwrite(dts_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, d.data()) < 0) 
        throw Exc("Error writing data to dataset");
    
    return *this;
}

Hdf5File &Hdf5File::Set(String name, const Vector<double> &d) {
	if (ExistDataset(name))
		Delete(name);
		
	hsize_t dims[1];
	dims[0] = d.size();
    HidS dataspace_id = H5Screate_simple(1, dims, NULL);
    if (dataspace_id < 0) 
        throw Exc("Error creating dataspace");
    
    if ((dts_id = H5Dcreate2(Last(group_ids), name, H5T_NATIVE_DOUBLE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        throw Exc("Error creating dataset");

    if (H5Dwrite(dts_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, d.begin()) < 0) 
        throw Exc("Error writing data to dataset");
    
    return *this;
}

Hdf5File &Hdf5File::Set(String name, const Eigen::MatrixXd &data) {
	if (ExistDataset(name))
		Delete(name);
		
	hsize_t dims[2];
	dims[0] = data.rows();
	dims[1] = data.cols();
    HidS dataspace_id = H5Screate_simple(2, dims, NULL);
    if (dataspace_id < 0) 
        throw Exc("Error creating dataspace");
    
    if ((dts_id = H5Dcreate2(Last(group_ids), name, H5T_NATIVE_DOUBLE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        throw Exc("Error creating dataset");
	
	Vector<double> d;
	CopyRowMajor(data, d);

    if (H5Dwrite(dts_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, d.begin()) < 0) 
        throw Exc("Error writing data to dataset");
    
    return *this;
}

Hdf5File &Hdf5File::Set(String name, const Buffer<double> &d, const MultiDimMatrixIndex &indx) {
	if (ExistDataset(name))
		Delete(name);
		
	Buffer<hsize_t> dims(indx.GetNumAxis());
	for (int i = 0; i < indx.GetNumAxis(); ++i)
		dims[i] = indx.size(i);
    HidS dataspace_id = H5Screate_simple(indx.GetNumAxis(), dims, NULL);
    if (dataspace_id < 0) 
        throw Exc("Error creating dataspace");
    
    if ((dts_id = H5Dcreate2(Last(group_ids), name, H5T_NATIVE_DOUBLE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        throw Exc("Error creating dataset");
	
    if (H5Dwrite(dts_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, d) < 0) 
        throw Exc("Error writing data to dataset");
    
    return *this;
}

String Hdf5File::GetLastError() {
	String str;

	auto WalkCallback = [](unsigned n, const H5E_error2_t *err_desc, void *client_data) {
		String *str = (String *)client_data;
		str->Cat(err_desc->desc);
	    return 0;
	};	
	herr_t eee = H5Ewalk2(H5E_DEFAULT, H5E_WALK_UPWARD, WalkCallback, &str);

    return str;
}


}