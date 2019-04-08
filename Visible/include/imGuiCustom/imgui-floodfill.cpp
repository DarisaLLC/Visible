#include <algorithm>
#include <limits>
#include <vector>
#include "imgui/imgui.h"

using namespace std;

#include "imgui-floodfill.h"

void PolyFillScanFlood(ImDrawList *draw, vector<ImVec2> *poly, ImColor color, int gap = 1, int strokeWidth = 1) {

	vector<ImVec2> scanHits;
	static ImVec2 min, max; // polygon min/max points
	auto io = ImGui::GetIO();
	double y;
	bool isMinMaxDone = false;
	unsigned int polysize = poly->size();

	// find the orthagonal bounding box
	// probably can put this as a predefined
	if (!isMinMaxDone) {
		min.x = min.y = DBL_MAX;
		max.x = max.y = DBL_MIN;
		for (auto p : *poly) {
			if (p.x < min.x) min.x = p.x;
			if (p.y < min.y) min.y = p.y;
			if (p.x > max.x) max.x = p.x;
			if (p.y > max.y) max.y = p.y;
		}
		isMinMaxDone = true;
	}

	// Bounds check
	if ((max.x < 0) || (min.x > io.DisplaySize.x) || (max.y < 0) || (min.y > io.DisplaySize.y)) return;

	// Vertically clip
	if (min.y < 0) min.y                = 0;
	if (max.y > io.DisplaySize.y) max.y = io.DisplaySize.y;

	// so we know we start on the outside of the object we step out by 1.
	min.x -= 1;
	max.x += 1;

	// Initialise our starting conditions
	y = min.y;

	// Go through each scan line iteratively, jumping by 'gap' pixels each time
	while (y < max.y) {

		scanHits.clear();

		{
			int jump = 1;
			ImVec2 fp = poly->at(0);

			for (size_t i = 0; i < polysize - 1; i++) {
				ImVec2 pa = poly->at(i);
				ImVec2 pb = poly->at(i+1);

				// jump double/dud points
				if (pa.x == pb.x && pa.y == pb.y) continue;

				// if we encounter our hull/poly start point, then we've now created the
				// closed
				// hull, jump the next segment and reset the first-point
				if ((!jump) && (fp.x == pb.x) && (fp.y == pb.y)) {
					if (i < polysize - 2) {
						fp   = poly->at(i + 2);
						jump = 1;
						i++;
					}
				} else {
					jump = 0;
				}

				// test to see if this segment makes the scan-cut.
				if ((pa.y > pb.y && y < pa.y && y > pb.y) || (pa.y < pb.y && y > pa.y && y < pb.y)) {
					ImVec2 intersect;

					intersect.y = y;
					if (pa.x == pb.x) {
						intersect.x = pa.x;
					} else {
						intersect.x = (pb.x - pa.x) / (pb.y - pa.y) * (y - pa.y) + pa.x;
					}
					scanHits.push_back(intersect);
				}
			}

			// Sort the scan hits by X, so we have a proper left->right ordering
			sort(scanHits.begin(), scanHits.end(), [](ImVec2 const &a, ImVec2 const &b) { return a.x < b.x; });

			// generate the line segments.
			{
				int i = 0;
				int l = scanHits.size() - 1; // we need pairs of points, this prevents segfault.
				for (i = 0; i < l; i += 2) {
					draw->AddLine(scanHits[i], scanHits[i + 1], color, strokeWidth);
				}
			}
		}
		y += gap;
	} // for each scan line
	scanHits.clear();
}

