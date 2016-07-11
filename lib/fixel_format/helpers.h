/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef __fixel_helpers_h__
#define __fixel_helpers_h__

#include "formats/mrtrix_utils.h"
#include "fixel_format/keys.h"

namespace MR
{
  namespace FixelFormat
  {
    template <typename IndexFileHeader>
    inline bool is_index_image (const IndexFileHeader& in)
    {
      return in.keyval ().count (n_fixels_key);
    }


    template <typename IndexFileHeader>
    inline void check_index_image (const IndexFileHeader& in)
    {
      if (!is_index_image (in))
        throw Exception (in.name () + " is not a valid fixel index image. Header key " + n_fixels_key + " not found");
    }


    template <typename DataFileHeader>
    inline bool is_data_image (const DataFileHeader& in)
    {
      return in.ndim () == 3 && in.size (2) == 1;
    }


    template <typename DataFileHeader>
    inline void check_data_image (const DataFileHeader& in)
    {
      if (!is_data_image (in))
        throw Exception (in.name () + " is not a valid fixel data image. Expected a 3-dimensionl image of size n x m x 1");
    }


    template <typename IndexFileHeader, typename DataFileHeader>
    inline bool fixels_match (const IndexFileHeader& index_h, const DataFileHeader& data_h)
    {
      bool fixels_match (false);

      if (is_index_image (index_h)) {
        fixels_match = std::stoul(index_h.keyval ().at (n_fixels_key)) == (unsigned long)data_h.size (0);
      }

      return fixels_match;
    }


    template <typename IndexFileHeader, typename DataFileHeader>
    inline void check_fixel_size (const IndexFileHeader& index_h, const DataFileHeader& data_h)
    {
      check_index_image (index_h);
      check_data_image (data_h);

      if (!fixels_match (index_h, data_h))
        throw Exception ("Fixel number mismatch between index image " + index_h.name() + " and data image " + data_h.name());
    }


    inline bool check_fixel_folder (const std::string &path, bool create_if_missing = false)
    {
      bool exists (true);

      if (!(exists = Path::exists (path))) {
        if (create_if_missing) File::mkdir (path);
        else throw Exception ("Fixel directory " + str(path) + " does not exist");
      }
      else if (!Path::is_dir (path))
        throw Exception (str(path) + " is not a directory");

      return exists;
    }


    inline Header find_index_header (const std::string &fixel_folder_path)
    {
      bool index_found (false);
      Header H;
      check_fixel_folder (fixel_folder_path);

      auto dir_walker = Path::Dir (fixel_folder_path);
      std::string fname;
      while ((fname = dir_walker.read_name ()).size ()) {
        auto full_path = Path::join (fixel_folder_path, fname);
        if (Path::has_suffix (fname, FixelFormat::supported_fixel_formats) && is_index_image (H = Header::open (full_path))) {
          index_found = true;
          break;
        }
      }

      if (!index_found)
        throw Exception ("Could not find index image in directory " + fixel_folder_path);

      return H;
    }


  }
}

#endif
