--require("cutorch")
require("torch")
require("nn")
require("image")
require("paths")

local IMAGE_HEIGHT = 16
local IMAGE_WIDTH = 32

print("Building datasets...")

local categories = {}

-- Store filenames
local images = {}
for category in paths.files("dataset/train") do
	if category ~= "." and category ~= ".." then
		table.insert(categories, category)
		for img in paths.files("dataset/train/"..category) do
			if img ~= "." and img ~= ".." then
				table.insert(images, {path = "dataset/train/"..category.."/"..img, label = #categories})
			end
		end
	end
end

local trainset = {data = torch.Tensor(#images, 3, IMAGE_HEIGHT, IMAGE_WIDTH), label = torch.Tensor(#images)}
local n_trainset = {}
for i, img in ipairs(images) do
	trainset.data[i] = image.load(img.path, 3, "double")
	trainset.label[i] = img.label
	n_trainset[categories[img.label]] = (n_trainset[categories[img.label]] or 0) + 1
end

setmetatable(trainset,
	{__index = function(t, i)
                return {t.data[i], t.label[i]}
        end}
);
function trainset:size()
	return self.data:size(1)
end

-- Normalization
local mean = {} -- store the mean, to normalize the test set in the future
local stdv  = {} -- store the standard-deviation for the future
for i=1,3 do -- over each image channel
	mean[i] = trainset.data[{ {}, {i}, {}, {}  }]:mean() -- mean estimation
	print('Channel ' .. i .. ', Mean: ' .. mean[i])
	trainset.data[{ {}, {i}, {}, {}  }]:add(-mean[i]) -- mean subtraction

	stdv[i] = trainset.data[{ {}, {i}, {}, {}  }]:std() -- std estimation
	print('Channel ' .. i .. ', Standard Deviation: ' .. stdv[i])
	trainset.data[{ {}, {i}, {}, {}  }]:div(stdv[i]) -- std scaling
end

-- Build the test dataset
local test_images = {}
for cat_i, category in ipairs(categories) do
	for img in paths.files("dataset/test/"..category) do
		if img ~= "." and img ~= ".." then
			table.insert(test_images, {path = "dataset/test/"..category.."/"..img, label = cat_i})
		end
	end
end
local testset = {data = torch.Tensor(#test_images, 3, IMAGE_HEIGHT, IMAGE_WIDTH), label = {}}
local n_testset = {}
for i, img in ipairs(test_images) do
	testset.data[i] = image.load(img.path, 3, "double")
	testset.label[i] = img.label
	n_testset[categories[img.label]] = (n_testset[categories[img.label]] or 0) + 1
end

-- Normalization
for i=1,3 do -- over each image channel
 	testset.data[{ {}, {i}, {}, {}  }]:add(-mean[i]) -- mean subtraction
	testset.data[{ {}, {i}, {}, {}  }]:div(stdv[i]) -- std scaling
end

local function sum(t)
	local s = 0
	for _, v in ipairs(t) do
		s = s + v
	end
	return s
end

io.write("Done.\n")

-- Construct the network
local width = IMAGE_WIDTH
local height = IMAGE_HEIGHT

local net = nn.Sequential()

net:add(nn.SpatialConvolution(3, 6, 3, 3))
width = math.floor(width - 3 + 1)
height = math.floor(height - 3 + 1)

net:add(nn.ReLU())

net:add(nn.SpatialMaxPooling(2, 2, 2, 2))
width = math.floor((width - 2) / 2 + 1)
height = math.floor((height - 2) / 2 + 1)

net:add(nn.SpatialConvolution(6, 16, 3, 3))
width = math.floor(width - 3 + 1)
height = math.floor(height - 3 + 1)

net:add(nn.ReLU())

net:add(nn.SpatialMaxPooling(2, 2, 2, 2))
width = math.floor((width - 2) / 2 + 1)
height = math.floor((height - 2) / 2 +1)

net:add(nn.View(16*width*height))
net:add(nn.Linear(16*width*height, 100))
net:add(nn.ReLU())
net:add(nn.Linear(100, 30))
net:add(nn.ReLU())
net:add(nn.Linear(30, #categories))
net:add(nn.LogSoftMax())
-- net = net:cuda()

print("\nTraining network...")

local criterion = nn.ClassNLLCriterion() -- Log-likelihood loss function
local trainer = nn.StochasticGradient(net, criterion)
trainer.learningRate = 0.001
trainer.maxIteration = 100
trainer.shuffleIndices = true
trainer:train(trainset)

print("\nTesting network...")

local results = {correct = 0, total = 0, prob_sum = 0, prob_min = math.huge, cat = {}}
for image = 1, testset.data:size(1) do
	-- io.write(string.format("-- Test image #%d\n", image))

	local truth = testset.label[image]
	local prediction = net:forward(testset.data[image])
	local confidences, indices = torch.sort(prediction, true)

	if not results.cat[truth] then
		results.cat[truth] = {correct = 0, total = 0,  prob_sum = 0, prob_min = math.huge}
	end

	results.total = results.total + 1
	results.cat[truth].total = results.cat[truth].total + 1
	if indices[1] == truth then
		local prob = math.exp(confidences[1])
		results.correct = results.correct + 1
		results.prob_sum = results.prob_sum + prob
		results.cat[truth].correct = results.cat[truth].correct + 1
		results.cat[truth].prob_sum = results.cat[truth].prob_sum + prob
		if prob < results.cat[truth].prob_min then
			results.cat[truth].prob_min = prob
			if prob < results.prob_min then
				results.prob_min = prob
			end
		end
	else
		print("Incorrect", test_images[image].path)
	end
end

io.write("\n-- Results --\n")
for i, cat in ipairs(categories) do
	io.write(string.format("%s: %d correct out of %d (%f%%). Mean confidence %f%%, min %f%%\n",
		cat, results.cat[i].correct, results.cat[i].total, results.cat[i].correct/results.cat[i].total*100,
		results.cat[i].prob_sum / results.cat[i].correct * 100, results.cat[i].prob_min * 100))
end
io.write(string.format("Total: %d correct out of %d (%f%%). Mean confidence %f%%, min %f%%\n",
	results.correct, results.total, results.correct/results.total*100,
	results.prob_sum / results.correct * 100, results.prob_min * 100))

torch.save("nnhornet.t7", {categories, {mean = mean, stdv = stdv}, net})
print("\nSaved model to nnhornet.t7. You can now run test.lua.")
