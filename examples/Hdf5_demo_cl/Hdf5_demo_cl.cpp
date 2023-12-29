// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2022, the Anboto author and contributors
#include <Core/Core.h>

using namespace Upp;

#include <Hdf5/hdf5.h>


void CreateDataset(String file) {
    herr_t status;

    hid_t file_id = H5Fcreate(file, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	
	hsize_t dims[2] = {4, 6};
    hid_t dataspace_id_double = H5Screate_simple(2, dims, NULL);
    hid_t dataset_id_double = H5Dcreate2(file_id, "/dset_double", H5T_NATIVE_DOUBLE, dataspace_id_double, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	status = H5Dclose(dataset_id_double);
	status = H5Sclose(dataspace_id_double);
	
    hid_t dataspace_id_int = H5Screate_simple(2, dims, NULL);
    hid_t dataset_id_int = H5Dcreate2(file_id, "/dset_int", H5T_NATIVE_INT, dataspace_id_int, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	status = H5Dclose(dataset_id_int);
	status = H5Sclose(dataspace_id_int);
	
	hsize_t dims_s = 1;
	hid_t dataspace_id_string = H5Screate_simple(1, &dims_s, NULL);	
	hid_t stringType = H5Tcopy(H5T_C_S1);
    H5Tset_size(stringType, H5T_VARIABLE);
    hid_t dataset_id_string = H5Dcreate2(file_id, "/dset_string", stringType, dataspace_id_string, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	status = H5Dclose(dataset_id_string);
	status = H5Sclose(dataspace_id_string);
	status = H5Tclose(stringType);
    
    status = H5Fclose(file_id);
}

void ReadDataset(String file) {
    herr_t status;

    hid_t file_id = H5Fopen(file, H5F_ACC_RDONLY, H5P_DEFAULT);

    hid_t dataset_id_double = H5Dopen2(file_id, "/dset_double", H5P_DEFAULT);
    double dset_data_double[4][6];
    status = H5Dread(dataset_id_double, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset_data_double);
    status = H5Dclose(dataset_id_double);
    VERIFY(dset_data_double[1][2] == 9.5);
    
    hid_t dataset_id_int = H5Dopen2(file_id, "/dset_int", H5P_DEFAULT);
    int dset_data_int[4][6];
    status = H5Dread(dataset_id_int, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset_data_int);
    status = H5Dclose(dataset_id_int);
	VERIFY(dset_data_int[1][2] == 9);

    hid_t dataset_id_string = H5Dopen2(file_id, "/dset_string", H5P_DEFAULT);
    hid_t dtype = H5Dget_type(dataset_id_string);
    hid_t space = H5Dget_space(dataset_id_string);
    hsize_t size = H5Sget_simple_extent_npoints(space);
    Buffer<char *> bstr(size);
    H5Dread(dataset_id_string, dtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, ~bstr);
    String str = String(bstr[0]);
    VERIFY(str == "Hello, HDF5!");    
    status = H5Dvlen_reclaim(dtype, space, H5P_DEFAULT, ~bstr);
    status = H5Dclose(dataset_id_string);
		
    status = H5Fclose(file_id);
}

void WriteDataset(String file) {
    herr_t status;
    double dset_data_double[4][6];
    int dset_data_int[4][6];

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 6; j++) {
            dset_data_double[i][j] = i * 6 + j + 1 + 0.5;
            dset_data_int[i][j] = int(dset_data_double[i][j]);
        }
    }

    hid_t file_id = H5Fopen(file, H5F_ACC_RDWR, H5P_DEFAULT);

    hid_t dataset_id_double = H5Dopen2(file_id, "/dset_double", H5P_DEFAULT);
    status = H5Dwrite(dataset_id_double, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset_data_double);
    status = H5Dclose(dataset_id_double);
    
    hid_t dataset_id_int = H5Dopen2(file_id, "/dset_int", H5P_DEFAULT);
    status = H5Dwrite(dataset_id_int, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset_data_int);
    status = H5Dclose(dataset_id_int);
    
    hid_t dataset_id_string = H5Dopen2(file_id, "/dset_string", H5P_DEFAULT);
    const char *str = "Hello, HDF5!";
 	hid_t stringType = H5Tcopy(H5T_C_S1);
    status = H5Tset_size(stringType, H5T_VARIABLE);
    status = H5Dwrite(dataset_id_string, stringType, H5S_ALL, H5S_ALL, H5P_DEFAULT, &str);
   	status = H5Dclose(dataset_id_string);
   	status = H5Tclose(stringType);
    
    status = H5Fclose(file_id);
}

void IterateDataset(hid_t group_id, String parent, int indentation) {
	herr_t status;
	
    // Iterate over objects in the group and identify datasets
    H5G_info_t group_info;
    H5Gget_info(group_id, &group_info);

	String sindentation(' ', indentation);
	
    for (hsize_t i = 0; i < group_info.nlinks; i++) {
        char obj_name[256];
        H5Lget_name_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, obj_name, sizeof(obj_name), H5P_DEFAULT);

        // Check if the object is a dataset
        if (H5Lexists(group_id, obj_name, H5P_DEFAULT) >= 0) {
            hid_t obj_id = H5Oopen(group_id, obj_name, H5P_DEFAULT);
            H5O_info2_t oinfo;
            if (H5Oget_info(obj_id, &oinfo, H5O_INFO_BASIC) >= 0) {
                String child = parent + "/" + obj_name;
            	if (oinfo.type == H5O_TYPE_GROUP) {
                	printf("%sGroup: %s\n", ~sindentation, ~child);
                	IterateDataset(obj_id, child, indentation+2);
            	} else if (oinfo.type == H5O_TYPE_DATASET) {
                	hid_t dspace = H5Dget_space(obj_id);
                	const int ndims = H5Sget_simple_extent_ndims(dspace);
                	Buffer<hsize_t> dims(ndims);
					H5Sget_simple_extent_dims(dspace, ~dims, NULL);
					String sdims;
					for (int id = 0; id < ndims; ++id) {
						if (id > 0)
							sdims += ",";	
						sdims += FormatInt(dims[id]);
					}
					if (ndims > 0)
						sdims = "[" + sdims + "]";
                	printf("%sDataset: %s%s", ~sindentation, ~child, ~sdims);
                	
                 	// Get the datatype of the dataset
				    hid_t datatype_id = H5Dget_type(obj_id);
				    if (datatype_id >= 0) {
					    // Determine the datatype class
					    H5T_class_t clss = H5Tget_class(datatype_id);
					    switch (clss) {
					   	case H5T_INTEGER:	printf("(int)");	break;
					   	case H5T_FLOAT:		printf("(float)");	break;
					    case H5T_STRING:	printf("(string)");	break;
					    default:			printf("(?)");
					    }
				    
					    hsize_t len = H5Dget_storage_size(obj_id);
					    for (int id = 0; id < ndims; ++id) 
							len /= dims[id];
						
					    printf(" %d ", len);
					    
					    if (ndims == 1 && clss == H5T_STRING) {
					        hsize_t size = H5Sget_simple_extent_npoints(dspace);
					    	Buffer<char *> bstr(size);
    						if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, ~bstr) >= 0) {
    							String str = String(bstr[0]);
    							Cout() << "'" << str << "'";
    						}
					    } else {
					        Cout() << "\n";     
					        
					        int sz = 1;
					        for (int id = 0; id < ndims; ++id) 
					            sz *= dims[id];
					        
					        if (clss == H5T_FLOAT) {
					            Buffer<double> d(sz);
					            if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, d) >= 0) {
					                for (int r = 0; r < dims[0]; ++r) {
					                    Cout() << sindentation << "  ";
					                    if (ndims > 1) {
					                		for (int c = 0; c < dims[1]; ++c)
					                			Cout() << d[r*dims[1] + c] << " ";
					                		Cout() << "\n";	
					                    } else
					                        Cout() << d[r] << " ";
					                }
					            }
					        } else if (clss == H5T_INTEGER) {
					            Buffer<int> d(sz);
					            if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, d) >= 0) {
					                for (int r = 0; r < dims[0]; ++r) {
					                    Cout() << sindentation << "  ";
					                    if (ndims > 1) {
					                		for (int c = 0; c < dims[1]; ++c)
					                			Cout() << d[r*dims[1] + c] << " ";
					                		Cout() << "\n";	
					                    } else
					                        Cout() << d[r] << " ";
					                }
					            }
					        } else  
					        	Cout() << "\n";     
					    }
				    } else
						Cout() << "\n";
            	} else if (oinfo.type == H5O_TYPE_NAMED_DATATYPE) 
                	printf("%sNamed data type: %s\n", ~sindentation, ~child);
            	else if (oinfo.type == H5O_TYPE_MAP) 
                	printf("%sMap: %s\n", ~sindentation, ~child);
            }
            H5Oclose(obj_id);
        }
    }
}
	
bool IterateDataset(String file) {
    // Open the HDF5 file
    hid_t file_id = H5Fopen(file, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) {
        fprintf(stderr, "Unable to open file.\n");
        return false;
    }

    // Open the root group (you can also open other groups if datasets are stored in subgroups)
    hid_t group_id = H5Gopen2(file_id, "/", H5P_DEFAULT);
    if (group_id < 0) {
        fprintf(stderr, "Unable to open root group.\n");
        H5Fclose(file_id);
        return false;
    }
	
	IterateDataset(group_id, "", 0);
	
    // Close the group and file
    H5Gclose(group_id);
    H5Fclose(file_id);

    return true;
}

CONSOLE_APP_MAIN
{
	String file = "data.h5";
	
	CreateDataset(file);
	WriteDataset(file);
	IterateDataset(file);
	ReadDataset(file);
	
	Cout() << "\nProgram ended";
	#ifdef flagDEBUG
	ReadStdIn();
	#endif
}




//Eigen::VectorXd v(dims[0]);
//if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, v.data()) >= 0) 

//	Eigen::MatrixXd m = Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(d, dims[0], dims[1]);
//	Cout() << m;
