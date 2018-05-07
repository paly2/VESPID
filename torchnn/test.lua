require("torch")
require("nn")
require("image")

local filename = ...
if not filename then
	print("Filename expected.")
	return 1
end

local categories, norm, net = unpack(torch.load("/usr/local/share/vespid/nnhornet.t7"))

-- First yield
coroutine.yield()

while true do
	local imgdata = image.load(filename, 3, "double")
	for i = 1, 3 do
		imgdata[{{i}, {}, {}}]:add(-norm.mean[i])
		imgdata[{{i}, {}, {}}]:div(norm.stdv[i])
	end

	local results = net:forward(imgdata)
	local confidences, indices = torch.sort(results, true)
	local exp_confidences = torch.exp(confidences)

	local t = {}
	for i = 1, indices:size(1) do
		t[categories[indices[i]]] = exp_confidences[i]
	end
	coroutine.yield(t)
end
