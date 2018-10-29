#pragma once


#include <memory>




class PixMap
{
public:
	PixMap(void)
		:width_(0)
		,height_(0)
		,step_(0)
		,stride_(0)
		,capacity_(0)
		,buffer_(nullptr)
		,ptr0_(0)
	{}
	PixMap(int width, int height, int step)
		:width_(width < 0 ? 0 : width)
		,height_(height < 0 ? 0 : height)
		,step_(step < 0 ? 0 : step)
		,stride_(step_*width_)
		,capacity_(stride_*height_)
		,buffer_(new uint8[capacity_])
		,ptr0_(buffer_.get())
	{}
	PixMap(int width, int height, int step, uint8* data, int stride = 0, bool copy = false)
		:width_(width)
		,height_(height)
		,step_(step)
		,stride_(stride ? stride : step_ * width)
		,capacity_(0) 
		,buffer_(nullptr)
		,ptr0_(data)
	{		
		if (copy)
			*this = clone();
	}

	int width(void)const { return width_; }
	int height(void)const { return height_; }
	int step(void)const { return step_; }
	int stride(void)const { return stride_; }
	bool const isContinuous(void) const { return stride_ == width_ * step_; }

	PixMap operator()(int x, int y, int width, int height) const
	{
		PixMap b(*this); 
		b.width_ = width;
		b.height_ = height;
		b.ptr0_ = b.ptr(y,x);
		return b;
	}

	void setTo(int value)
	{
		int h = height_;
		int w = width_ * step_;
		if (w == stride_)
		{
			w *= h;
			h = 1;
		}

		for (int y = 0; y < h; ++y)
		{
			uint8* p = ptr(y);
			for (int x = 0; x < w; ++x)
				p[x] = value;
		}
	}

	void create(int width, int height, int step)
	{
		if (step_ == step && width_ == width &&	height_ == height)
			return;
		*this = PixMap(width, height, step);
	}

	void copyTo(PixMap& other) const
	{
		other.create(width_, height_, step_);

		int h = height_;
		int w = width_ * step_;
		if (w == stride_ && w == other.stride_)
		{
			w *= h;
			h = 1;
		}

		for (int y = 0; y < h; ++y)
		{
			const uint8* p = ptr(y);
			std::copy(p, p + w, other.ptr(y));
		}
	}
	PixMap clone(void) const
	{
		PixMap b;
		copyTo(b);
		return b;
	}


	template<typename T = uint8>
	T* ptr(int y = 0, int x = 0)
	{
		return (T*)(ptr0_ + stride_ * y + step_ * x);
	}
	template<typename T = uint8>
	const T* ptr(int y = 0, int x = 0) const
	{
		return (T*)(ptr0_ + stride_ * y + step_ * x);
	}

	//bool isRegion(void) const
	//{
	//	return ptr0_ != buffer_.get() || height_ * stride_ != capacity_;
	//}
	//bool isReference(void) const
	//{
	//	return !buffer_.get() && ptr0_;
	//}

	bool isOverlapping(const PixMap& other) const
	{		
		if (ptr0_ < other.ptr0_)
			return is_overlap(*this, other);
		else if (ptr0_ > other.ptr0_)
			return is_overlap(other, *this);
		else 
			return ptr0_ != nullptr;
	}
private:
	static bool is_overlap(const PixMap& lo, const PixMap& hi)
	{
		if (lo.ptr(lo.height_ - 1, lo.width_ - 1) < hi.ptr0_)
			return false;
		int x = (hi.ptr0_ - lo.ptr0_) % lo.stride_;
		return (x < lo.width_*lo.step_ || x + hi.width_*hi.step_ > lo.stride_);
	}
	static int alignRow(int step, int width, int row_alignment_power_2)
	{
		int m = (1 << row_alignment_power_2) - 1; // 0 1 3 7 15 31 etc
		return (step*width + m) & ~m;
	}

	int width_;
	int height_;
	int step_;
	int stride_;
	size_t capacity_;
	std::shared_ptr<uint8[]> buffer_;
	uint8* ptr0_;
};