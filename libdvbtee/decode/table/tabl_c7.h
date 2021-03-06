/*****************************************************************************
 * Copyright (C) 2011-2015 Michael Ira Krufky
 *
 * Author: Michael Ira Krufky <mkrufky@linuxtv.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  c7110-1301  USA
 *
 *****************************************************************************/

#ifndef _MGT_H__
#define _MGT_H__

#include "table.h"

#ifndef _ATSC_MGT_H
#include "dvbpsi/atsc_mgt.h"
#endif

#include "decode.h"

namespace dvbtee {

namespace decode {

/* MGT (Table) */

class mgt: public Table/*<dvbpsi_atsc_mgt_t>*/ {
TABLE_DECODER_TPL
public:
	mgt(Decoder *, TableWatcher*);
	mgt(Decoder *, TableWatcher*, const dvbpsi_atsc_mgt_t * const);
	virtual ~mgt();

	void store(const dvbpsi_atsc_mgt_t * const);

	static bool ingest(TableStore *s, const dvbpsi_atsc_mgt_t * const t, TableWatcher *w = NULL);

	const decoded_mgt_t& getDecodedMGT() const { return decoded_mgt; }

private:
	decoded_mgt_t   decoded_mgt;
};

class mgtTb: public TableDataComponent {
public:
	mgtTb(decoded_mgt_t&, Decoder*, const dvbpsi_atsc_mgt_table_t * const);
	virtual ~mgtTb();
};

}

}

#endif /* _MGT_H__ */
