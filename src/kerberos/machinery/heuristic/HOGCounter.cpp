//
// Created by Richard Wollack on 4/3/17.
//

#include "machinery/heuristic/HOGCounter.h"

namespace kerberos
{
    void HOGCounter::setup(const StringMap & settings)
    {
        std::vector<std::string> coordinates;

        // Set incoming coordinates
        helper::tokenize(settings.at("heuristics.Counter.markers"), coordinates, "|");
        for(int i = 0; i < 2; i++)
        {
            std::vector<std::string> fromAndTo;
            helper::tokenize(coordinates[i], fromAndTo, ",");
            int from = std::atoi(fromAndTo[0].c_str());
            int to = std::atoi(fromAndTo[1].c_str());
            cv::Point p(from ,to);
            m_in.push_back(p);
        }

        // Set outgoing coordinates

        for(int i = 2; i < 4; i++)
        {
            std::vector<std::string> fromAndTo;
            helper::tokenize(coordinates[i], fromAndTo, ",");
            int from = std::atoi(fromAndTo[0].c_str());
            int to = std::atoi(fromAndTo[1].c_str());
            cv::Point p(from ,to);
            m_out.push_back(p);
        }

        setMinimumChanges(std::atoi(settings.at("heuristics.Counter.minimumChanges").c_str()));
        setNoMotionDelayTime(std::atoi(settings.at("heuristics.Counter.noMotionDelayTime").c_str()));
        setAppearance(std::atoi(settings.at("heuristics.Counter.appearance").c_str()));
        setMaxDistance(std::atoi(settings.at("heuristics.Counter.maxDistance").c_str()));
        setMinArea(std::atoi(settings.at("heuristics.Counter.minArea").c_str()));
        setOnlyTrueWhenCounted((settings.at("heuristics.Counter.onlyTrueWhenCounted") == "true"));
        setWinStrideX(std::atoi(settings.at("heuristics.HOGCounter.winStrideX").c_str()));
        setWinStrideY(std::atoi(settings.at("heuristics.HOGCounter.winStrideY").c_str()));
        setPaddingX(std::atoi(settings.at("heuristics.HOGCounter.paddingX").c_str()));
        setPaddingY(std::atoi(settings.at("heuristics.HOGCounter.paddingY").c_str()));
        setFinalThreshold(std::atof(settings.at("heuristics.HOGCounter.finalThreshold").c_str()));
        setScale(std::atof(settings.at("heuristics.HOGCounter.scale").c_str()));
    }

    bool HOGCounter::intersection(cv::Point2f o1, cv::Point2f p1, cv::Point2f o2, cv::Point2f p2, cv::Point2f &r)
    {
        LINFO << "checking intersection: o1: " << o1 << " o2: " << o2 << " p1: " << p1 << " p2: " << p2;
        cv::Point2f x = o2 - o1;
        cv::Point2f d1 = p1 - o1;
        cv::Point2f d2 = p2 - o2;

        float cross = d1.x * d2.y - d1.y * d2.x;

        if (std::abs(cross) < 1e-8)
        {
            return false;
        }

        double t1 = (x.x * d2.y - x.y * d2.x)/cross;
        r = o1 + d1 * t1;

        if((r.x <= MAX(o1.x, p1.x) && r.x >= MIN(o1.x, p1.x) && r.y <= MAX(o1.y, p1.y) && r.y >= MIN(o1.y, p1.y)) &&
           (r.x <= MAX(o2.x, p2.x) && r.x >= MIN(o2.x, p2.x) && r.y <= MAX(o2.y, p2.y) && r.y >= MIN(o2.y, p2.y)))
        {
            return true;
        }

        return false;
    }

    bool HOGCounter::isValid(const Image & evaluation, const ImageVector & images, JSON & data)
    {
        int numberOfChanges;
        Rectangle rectangle;
        numberOfChanges = data["numberOfChanges"].GetInt();

        int incoming = 0;
        int outgoing = 0;

        kerberos::Image image = evaluation;
        cv::Mat img = image.getImage();
//        cv::dilate(img, img, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(25,25)));
//        cv::erode(img, img, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(10,10)));

        cv::Point & outTop = m_out[0];
        cv::Point & outBottom = m_out[1];

        cv::Point & inTop = m_in[0];
        cv::Point & inBottom = m_in[1];

//        std::vector<std::vector<cv::Point> > contours;
//        std::vector<cv::Vec4i> hierarchy;
//        cv::findContours(image.getImage(), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
        std::vector<cv::Rect> found, found_filtered;
        cv::HOGDescriptor hog;
        hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());
        cv::Mat evalImage = images[2]->getImage();

        hog.detectMultiScale(evalImage,
                             found,
                             0,
                             cv::Size(m_winStrideX,m_winStrideY),
                             cv::Size(m_paddingX,m_paddingY),
                             m_scale,
                             m_finalThreshold);
//        hog.detectMultiScale(image.getImage(), found, 0, cv::Size(16,16), cv::Size(32,32), 1.059, 1.5);

        size_t i, j;
        for (i=0; i<found.size(); i++)
        {
            cv::Rect r = found[i];
            for (j=0; j<found.size(); j++)
                if (j!=i && (r & found[j]) == r)
                    break;
            if (j== found.size()) {
                found_filtered.push_back(r);
            }
        }

        LINFO << "Number of rects in HOG: " << found_filtered.size();
//
        JSON::AllocatorType& allocator = data.GetAllocator();
        data.AddMember("incoming", found_filtered.size(), allocator);
        data.AddMember("outgoing", found_filtered.size(), allocator);

        return found_filtered.size() > 0;

//        for (i=0; i<found_filtered.size(); i++)
//        {
//            cv::Rect r = found_filtered[i];
//            r.x += cvRound(r.width*0.1);
//            r.width = cvRound(r.width*0.8);
//            r.y += cvRound(r.height*0.07);
//            r.height = cvRound(r.height*0.8);
//            cv::rectangle(evalImage, r.tl(), r.br(), cv::Scalar(0,255,0), 3);
//        }

//        int numberOfContours= 0;
//        for(int i = 0; i < m_features.size(); i++)
//        {
//            int mostRecent = m_features[i].size() - 1;
//            m_features[i][mostRecent].decreaseAppearance();
//        }
//
//        // Find new tracks
//        if(found_filtered.size() > 0)
//        {
//            for( int i = 0; i< found_filtered.size(); i++ )
//            {
////                cv::Moments moments = cv::moments(contours[i]);
////                int x = moments.m10/moments.m00;
////                int y = moments.m01/moments.m00;
////                int area = cv::contourArea(contours[i]);
//                cv::Point center_of_rect = (found_filtered[i].br() + found_filtered[i].tl())*0.5;
//                int x = center_of_rect.x;
//                int y = center_of_rect.y;
//                int area = found_filtered[i].area();
//
//                if(area < m_minArea) continue;
//
//                Feature current(x, y, area, m_appearance);
//                int best = -1;
//                double bestValue = 99999999;
//                double bestArea = 99999999;
//
//                for(int j = 0; j < m_features.size(); j++)
//                {
//                    int mostRecent = m_features[j].size() - 1;
//                    double distance = current.distance(m_features[j][mostRecent]);
//                    double areaDistance = current.areaDistance(m_features[j][mostRecent]);
//
//                    if(distance < m_maxDistance && distance < bestValue + m_maxDistance/2)
//                    {
//                        if(areaDistance < bestArea)
//                        {
//                            best = j;
//                            bestValue = distance;
//                        }
//                    }
//                }
//
//                if(best == -1)
//                {
//                    std::vector<Feature> tracking;
//                    tracking.push_back(current);
//                    m_features.push_back(tracking);
//                }
//                else
//                {
//                    m_features[best].push_back(current);
//                }
//
//                numberOfContours++;
//            }
//        }
//
//        // Remove old tracks
//        std::vector<std::vector<Feature> >::iterator it = m_features.begin();
//        while(it != m_features.end())
//        {
//            Feature & back = it->back();
//            back.decreaseAppearance();
//
//            if(back.getAppearance() < 0)
//            {
//                it = m_features.erase(it);
//            }
//            else
//            {
//                it++;
//            }
//        }
//
//        // Check existing tracks if they crossed the line
//        LINFO << "Number of features in HOG: " << m_features.size();
//        it = m_features.begin();
//        while(it != m_features.end())
//        {
//            if(it->size() > 1)
//            {
//                for(int j = 1; j < it->size(); j++)
//                {
//                    cv::Point2f prev((*it)[j-1].getX(),(*it)[j-1].getY());
//                    cv::Point2f curr((*it)[j].getX(),(*it)[j].getY());
//                }
//
//                // Check if cross line
//                cv::Point2f start((*it)[0].getX(),(*it)[0].getY());
//                cv::Point2f end((*it)[it->size()-1].getX(),(*it)[it->size()-1].getY());
//
//                // Check if interset in line
//                cv::Point2f intersectionPointOutgoing;
//                bool inLine = false;
//                if(intersection(start, end, outTop, outBottom, intersectionPointOutgoing))
//                {
//                    inLine = true;
//                }
//                // Check if interset out line
//                cv::Point2f intersectionPointIncoming;
//                bool outLine = false;
//                if(intersection(start, end, inTop, inBottom, intersectionPointIncoming))
//                {
//                    outLine = true;
//                }
//
//                // Check if interesected both
//                Direction xDirection = parallell;
//                Direction yDirection = parallell;
//
//                if(inLine && outLine)
//                {
//                    // What is the direction (incoming our outgoing?)
//                    if(start.x - end.x < 0)
//                    {
//                        xDirection = right;
//                    }
//                    else if(start.x - end.x > 0)
//                    {
//                        xDirection = left;
//                    }
//                    if(start.y - end.y < 0)
//                    {
//                        yDirection = bottom;
//                    }
//                    else if(start.y - end.y > 0)
//                    {
//                        yDirection = top;
//                    }
//
//                    // Check which intersection point comes first
//                    if(xDirection != parallell)
//                    {
//                        if(xDirection == left)
//                        {
//                            // Check which intersection point is most right;
//                            if(intersectionPointIncoming.x > intersectionPointOutgoing.x)
//                            {
//                                incoming++;
//                            }
//                            else
//                            {
//                                outgoing++;
//                            }
//                        }
//                        else if(xDirection == right)
//                        {
//                            // Check which intersection point is most right;
//                            if(intersectionPointIncoming.x < intersectionPointOutgoing.x)
//                            {
//                                incoming++;
//                            }
//                            else
//                            {
//                                outgoing++;
//                            }
//                        }
//                    }
//                    else
//                    {
//
//                    }
//
//                    it = m_features.erase(it);
//                }
//                else
//                {
//                    it++;
//                }
//            }
//            else
//            {
//                it++;
//            }
//        }
//
//        if(numberOfChanges >= m_minimumChanges)
//        {
//            JSON::AllocatorType& allocator = data.GetAllocator();
//            data.AddMember("incoming", incoming, allocator);
//            data.AddMember("outgoing", outgoing, allocator);
//
//            if(m_onlyTrueWhenCounted)
//            {
//                if(incoming > 0 || outgoing > 0)
//                {
//                    BINFO << "HOGCounter: in (" << helper::to_string(incoming) << "), out (" << helper::to_string(outgoing) << ")";
//                    return true;
//                }
//            }
//            else
//            {
//                return true;
//            }
//        }
//        else
//        {
//            usleep(m_noMotionDelayTime*1000);
//        }
//
//        return false;
    }
}
