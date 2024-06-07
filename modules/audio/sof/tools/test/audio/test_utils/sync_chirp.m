function [y, mark]= sync_chirp(fs, direction)

%% [y, mark]= sync_chirp(fs, direction)
%
% Returns a chirp signal that can be used to mark end and beginning of audio
% tests to synchronize input and captured output accurately.
%
% Input
% fs - sample rate
% direction - direction of frequency sweep 'up' or 'down'
%
% Output
% y - windowed chirp signal
% mark - struct with parameters for chirp
%

%%
% Copyright (c) 2017, Intel Corporation
% All rights reserved.
%
% Redistribution and use in source and binary forms, with or without
% modification, are permitted provided that the following conditions are met:
%   * Redistributions of source code must retain the above copyright
%     notice, this list of conditions and the following disclaimer.
%   * Redistributions in binary form must reproduce the above copyright
%     notice, this list of conditions and the following disclaimer in the
%     documentation and/or other materials provided with the distribution.
%   * Neither the name of the Intel Corporation nor the
%     names of its contributors may be used to endorse or promote products
%     derived from this software without specific prior written permission.
%
% THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
% AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
% IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
% ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
% LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
% CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
% SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
% INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
% CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
% ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
% POSSIBILITY OF SUCH DAMAGE.
%
% Author: Seppo Ingalsuo <seppo.ingalsuo@linux.intel.com>
%

mark.a_db = -10;
mark.a = 10^(mark.a_db/20);
mark.fs = fs;
mark.t = 0.2; % Short, to not consume much test time
switch lower(direction)
        case 'up'
                mark.f0 = 200; % Should work with any speaker
                mark.f1 = 3500; % Should work with any speaker
        case 'down'
                mark.f1 = 200; % Should work with any speaker
                mark.f0 = 3500; % Should work with any speaker
        otherwise
                error('Parameter direction must be ''up'' or ''down''');
end

mark.n = round(mark.t*fs);
t_vec = (0:mark.n-1)/fs;
w = hanning(mark.n);
x = chirp(t_vec, mark.f0, mark.t, mark.f1, 'linear')';
y = x .* w * mark.a;

end
