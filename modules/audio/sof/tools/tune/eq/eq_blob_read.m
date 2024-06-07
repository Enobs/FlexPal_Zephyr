function blob = eq_blob_read(blobfn, fntype)

% SPDX-License-Identifier: BSD-3-Clause
%
% Copyright (c) 2020, Intel Corporation. All rights reserved.
%
% Author: Seppo Ingalsuo <seppo.ingalsuo@linux.intel.com>

%% Use file suffix as type
if nargin < 2
	idx = findstr(blobfn, '.');
	fntype = blobfn(idx(end)+1:end);
end

%% Read the file
switch lower(fntype)
	case 'bin'
		fh = fopen(blobfn, 'rb');
		tmp = fread(fh, inf, 'uint32');
		fclose(fh);
	case 'txt'
		fh = fopen(blobfn, 'r');
		tmp = fscanf(fh, '%u,', Inf);
		fclose(fh);
	case 'm4'
		tmp = get_parse_m4(blobfn);
	otherwise
		error('Illegal file type, please give fntype argument');

end

blob = uint32(tmp);

end

%% Parse m4 topology blob
function blob = get_parse_m4(blobfn)

fh = fopen(blobfn, 'r');

% Ignore two lines from beginning
ln = fgets(fh);
ln = fgets(fh);

% Loop until end of file
n = 1;
ln = fgets(fh);
while ln ~= -1
	idx = findstr(ln, '0x');
	for i = 1:length(idx)
		bytes(n) = hex2dec(ln(idx(i)+2:idx(i)+3));
		n = n + 1;
	end
	ln = fgets(fh);
end
fclose(fh);

% Convert to 32 bit
n32 = round(length(bytes)/4);
blob = zeros(n32, 1);
for i = 1:n32
	i8 = (i - 1)*4 + 1;
	blob(i) = bytes(i8) * 2^0 ...
		  + bytes(i8 + 1) * 2^8 ...
		  + bytes(i8 + 2) * 2^16 ...
		  + bytes(i8 + 3) * 2^24;
end

end
