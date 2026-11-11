package com.ilu.game;

import android.os.Bundle;

public class MainActivity extends android.app.NativeActivity
{
	private volatile boolean firstFrameRendered = false;

	static
	{
		// Here we can load libraries before onCreate is invoked
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// The splash screen stays until the window draws its first frame. NativeActivity
		// reports its window as drawn right away, so cancel the window draws (they are
		// retried every frame) until native code has rendered its first frame.
		findViewById(android.R.id.content).getViewTreeObserver().addOnPreDrawListener(() -> firstFrameRendered);
	}

	// Called from native code once the first frame has been rendered
	public void onFirstFrameRendered()
	{
		firstFrameRendered = true;
	}
}
