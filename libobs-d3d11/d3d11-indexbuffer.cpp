/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "d3d11-subsystem.hpp"

void gs_index_buffer::InitBuffer()
{
	D3D11_BUFFER_DESC bd;
	D3D11_SUBRESOURCE_DATA srd;
	HRESULT hr;

	memset(&bd,  0, sizeof(bd));
	memset(&srd, 0, sizeof(srd));

	bd.Usage          = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
	bd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
	bd.ByteWidth      = UINT(indexSize * num);
	srd.pSysMem       = indices.data;

	hr = device->device->CreateBuffer(&bd, &srd, indexBuffer.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create buffer", hr);
}

gs_index_buffer::gs_index_buffer(device_t device, enum gs_index_type type,
		void *indices, size_t num, uint32_t flags)
	: device  (device),
	  type    (type),
	  indices (indices),
	  num     (num),
	  dynamic ((flags & GS_DYNAMIC) != 0)
{
	switch (type) {
	case GS_UNSIGNED_SHORT: indexSize = 2; break;
	case GS_UNSIGNED_LONG:  indexSize = 4; break;
	}

	InitBuffer();
}
